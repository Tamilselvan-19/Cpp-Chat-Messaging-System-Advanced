#include "BackendManager.h"

#include <chrono>
#include <string>

#include "../Common/Logger.h"

void BackendManager::AddBackend(std::unique_ptr<Backend> backend) {
    m_backends.push_back(std::move(backend));
}

Backend* BackendManager::NextHealthy() {
    if (m_backends.empty()) return nullptr;

    for (size_t i = 0; i < m_backends.size(); ++i) {
        auto idx = (m_rr.fetch_add(1) + i) % m_backends.size();
        Backend* b = m_backends[idx].get();
        if (b && b->healthy.load()) return b;
    }
    return nullptr;
}

bool BackendManager::Probe(const Backend& backend, int timeoutMs) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(backend.tcpPort));
    inet_pton(AF_INET, backend.host.c_str(), &addr.sin_addr);

    int opt = timeoutMs;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&opt), sizeof(opt));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&opt), sizeof(opt));

    bool ok = (connect(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR);
    closesocket(s);
    return ok;
}

void BackendManager::StartHealthChecks(int intervalMs, int probeTimeoutMs) {
    m_running = true;
    m_healthThread = std::thread([this, intervalMs, probeTimeoutMs] {
        HealthCheckLoop(intervalMs, probeTimeoutMs);
    });
}

void BackendManager::Stop() {
    m_running = false;
    if (m_healthThread.joinable()) m_healthThread.join();
}

void BackendManager::HealthCheckLoop(int intervalMs, int probeTimeoutMs) {
    while (m_running) {
        for (auto& backend : m_backends) {
            if (!backend) continue;
            bool ok = Probe(*backend, probeTimeoutMs);
            backend->healthy = ok;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}
