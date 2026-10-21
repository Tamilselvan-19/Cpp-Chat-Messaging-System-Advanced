#pragma once

#include <atomic>
#include <map>
#include <string>
#include <thread>

class UdpHeartbeat {
public:
    bool Start(const ServerConfig& config);
    void Stop();
    void NoteUserConnected(const std::string& userId, const std::string& ip);

private:
    void UdpLoop();
    void CleanupExpired();

    ServerConfig m_cfg;
    SOCKET m_udpSocket = INVALID_SOCKET;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::map<std::string, std::pair<std::string, long long>> m_users;
    std::mutex m_mutex;
};
