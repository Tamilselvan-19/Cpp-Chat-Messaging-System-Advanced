#include "LoadBalancer.h"

#include <map>
#include <ws2tcpip.h>

#include "../Common/Ini.h"
#include "../Common/JsonUtil.h"
#include "../Common/Logger.h"
#include "../Common/Protocol.h"
#include "TcpProxy.h"

bool LoadBalancer::LoadConfig(const std::string& path) {
    m_listenPort = IniFile::ReadInt(path, "balancer", "ListenPort", 5000);
    m_udpPort = IniFile::ReadInt(path, "balancer", "UdpPort", 5001);
    m_healthIntervalMs = IniFile::ReadInt(path, "balancer", "HealthCheckIntervalMs", 2000);
    m_healthTimeoutMs = IniFile::ReadInt(path, "balancer", "HealthCheckTimeoutMs", 1000);
    m_logFile = IniFile::ReadValue(path, "balancer", "LogFile", "logs/loadbalancer.log");

    for (const char* section : { "backend1", "backend2" }) {
        std::string name = IniFile::ReadValue(path, section, "Name", section);
        std::string host = IniFile::ReadValue(path, section, "Host", "127.0.0.1");
        int tcp = IniFile::ReadInt(path, section, "TcpPort", 0);
        int udp = IniFile::ReadInt(path, section, "UdpPort", 0);
        if (tcp == 0) continue;

        auto b = std::make_unique<Backend>();
        b->name = name;
        b->host = host;
        b->tcpPort = tcp;
        b->udpPort = udp;
        b->healthy = true;

        m_manager.AddBackend(std::move(b));
    }

    return !m_manager.Backends().empty();
}

bool LoadBalancer::Start(const std::string& configPath) {
    if (!LoadConfig(configPath)) {
        std::cerr << "No healthy backends configured" << std::endl;
        return false;
    }

    Logger::Init(m_logFile);

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) return false;

    int yes = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(m_listenPort));

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
        listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        Logger::Error("TCP      : bind/listen failed");
        return false;
    }

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSocket != INVALID_SOCKET) {
        sockaddr_in uaddr{};
        uaddr.sin_family = AF_INET;
        uaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        uaddr.sin_port = htons(static_cast<u_short>(m_udpPort));
        if (bind(m_udpSocket, reinterpret_cast<sockaddr*>(&uaddr), sizeof(uaddr)) == SOCKET_ERROR) {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET;
        }
    }

    m_manager.StartHealthChecks(m_healthIntervalMs, m_healthTimeoutMs);
    m_running = true;
    m_acceptThread = std::thread([this] { AcceptLoop(); });

    if (m_udpSocket != INVALID_SOCKET) {
        m_udpThread = std::thread([this] { UdpRelayLoop(); });
    }

    Logger::Info("Load balancer started");
    return true;
}

void LoadBalancer::Stop() {
    m_running = false;
    m_manager.Stop();

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_udpSocket != INVALID_SOCKET) {
        closesocket(m_udpSocket);
        m_udpSocket = INVALID_SOCKET;
    }

    if (m_acceptThread.joinable()) m_acceptThread.join();
    if (m_udpThread.joinable()) m_udpThread.join();

    Logger::Info("Load balancer stopped.");
    Logger::Close();
}

void LoadBalancer::AcceptLoop() {
    while (m_running) {
        sockaddr_in from{};
        int fromLen = sizeof(from);

        SOCKET clientSocket = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (clientSocket == INVALID_SOCKET) {
            if (m_running) Logger::Error("TCP : accept() failed");
            break;
        }

        char ip[64] = {};
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
        std::string clientIp = ip;

        Backend* backend = m_manager.NextHealthy();
        if (!backend) {
            Logger::Error("Routing : no healthy backend");
            protocol::SendFrame(clientSocket,
                JsonBuild({ {"type", "error"}, {"message", "No healthy backend"} }));
            closesocket(clientSocket);
            continue;
        }

        SOCKET backendSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in baddr{};
        baddr.sin_family = AF_INET;
        inet_pton(AF_INET, backend->host.c_str(), &baddr.sin_addr);
        baddr.sin_port = htons(static_cast<u_short>(backend->tcpPort));

        if (connect(backendSocket, reinterpret_cast<sockaddr*>(&baddr), sizeof(baddr)) == SOCKET_ERROR) {
            Logger::Error("Routing : backend connect failed");
            backend->healthy = false;
            closesocket(backendSocket);
            closesocket(clientSocket);
            continue;
        }

        std::string firstFrame;
        if (protocol::RecvFrame(clientSocket, firstFrame, 10000)) {
            std::map<std::string, std::string> auth;
            if (JsonParse(firstFrame, auth) && JsonGet(auth, "type") == "auth") {
                std::string uid = JsonGet(auth, "userId");
                if (!uid.empty()) {
                    std::lock_guard<std::mutex> lock(m_udpRouteMutex);
                    m_udpRouteByUser[uid] = backend->udpPort;
                    m_udpBackendHost[uid] = backend->host;
                }
            }
            protocol::SendFrame(backendSocket, firstFrame);
        }

        DWORD noTimeout = 0;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<const char*>(&noTimeout), sizeof(noTimeout));

        auto session = std::make_shared<ProxySession>();
        session->clientSocket = clientSocket;
        session->backendSocket = backendSocket;
        session->clientIp = clientIp;
        session->backend = backend;

        TcpProxy::StartProxyPair(session);
    }
}

void LoadBalancer::UdpRelayLoop() {
    char buffer[2048];
    while (m_running) {
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int n = recvfrom(m_udpSocket, buffer, sizeof(buffer) - 1, 0,
            reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n <= 0) continue;

        buffer[n] = '\0';

        std::map<std::string, std::string> hb;
        if (!JsonParse(buffer, hb)) continue;

        std::string userId = JsonGet(hb, "userId");
        if (userId.empty()) continue;

        int port = 0;
        std::string host;

        {
            std::lock_guard<std::mutex> lock(m_udpRouteMutex);
            auto it = m_udpRouteByUser.find(userId);
            if (it == m_udpRouteByUser.end()) continue;
            port = it->second;
            host = m_udpBackendHost[userId];
        }

        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, host.c_str(), &dest.sin_addr);
        sendto(m_udpSocket, buffer, n, 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    }
}
