#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "ServerConfig.h"

struct redisContext;
struct redisReply;

class MySQLDatabase;

class RedisCache {
public:
    RedisCache(const ServerConfig& config, MySQLDatabase* db);
    ~RedisCache();

    bool Connect();
    bool IsConnected() const { return m_connected; }
    bool ValidateUser(const std::string& userId, const std::string& password,
                      const std::string& ip, bool& cacheHit);
    bool Publish(const std::string& channel, const std::string& message);
    void StartSubscriber(const std::string& channel,
                         std::function<void(const std::string&)> onMessage);
    void Stop();

private:
    bool ValidateViaSql(const std::string& userId, const std::string& password,
                        const std::string& ip, bool& cacheHit);
    void CacheUser(const std::string& userId, const std::string& passwordHash, const std::string& ip);
    bool ReadCachedHash(const std::string& userId, std::string& hashOut);
    void SubscriberLoop(const std::string& channel,
                        std::function<void(const std::string&)> onMessage);

    ServerConfig m_cfg;
    MySQLDatabase* m_db = nullptr;
    redisContext* m_ctx = nullptr;
    redisContext* m_subCtx = nullptr;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    std::thread m_subThread;
};
