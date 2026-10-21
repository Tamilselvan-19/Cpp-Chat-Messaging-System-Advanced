#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>

struct ServerConfig {
    std::string serverId = "server1";
    int tcpPort = 6001;
    int udpPort = 7001;
    std::string redisHost = "127.0.0.1";
    int redisPort = 6379;
    std::string kafkaBrokers = "127.0.0.1:9092";
    std::string kafkaTopic = "chat-messages";
    std::string mysqlHost = "127.0.0.1";
    int mysqlPort = 3306;
    std::string mysqlDb = "ChatDatabase";
    std::string mysqlUser = "root";
    std::string mysqlPassword = "";
    std::string logFile = "logs/server.log";
    int heartbeatIntervalMs = 5000;
    int inactivityTimeoutMs = 15000;
    int cacheTTLSeconds = 300;

    static ServerConfig Load(const std::string& path);
};

class RedisCache;
class KafkaProducer;
class UdpHeartbeat;

struct ClientInfo {
    SOCKET socket = INVALID_SOCKET;
    std::string userId;
    std::string ip;
    std::atomic<bool> alive{true};
};

class TcpServer {
public:
    TcpServer(const ServerConfig& config, RedisCache* redis,
              KafkaProducer* kafka, UdpHeartbeat* heartbeat);
    ~TcpServer();

    bool Start();
    void Stop();
    void OnCrossServerMessage(const std::string& payload);

private:
    void AcceptLoop();
    void ClientLoop(std::shared_ptr<ClientInfo> client);
    void BroadcastLocal(const std::string& payload, const std::string& excludeUserId);

    ServerConfig m_cfg;
    RedisCache* m_redis = nullptr;
    KafkaProducer* m_kafka = nullptr;
    UdpHeartbeat* m_heartbeat = nullptr;
    SOCKET m_listenSocket = INVALID_SOCKET;
    std::atomic<bool> m_running{false};
    std::thread m_acceptThread;
    std::mutex m_clientsMutex;
    std::map<std::string, std::shared_ptr<ClientInfo>> m_clients;
};
