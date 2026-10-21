#include "ServerConfig.h"

#include <fstream>
#include <sstream>
#include <string>

#include "../Common/Ini.h"

ServerConfig ServerConfig::Load(const std::string& path) {
    ServerConfig cfg;
    cfg.serverId = ini::ReadValue(path, "server", "server_id", cfg.serverId);
    cfg.tcpPort = ini::ReadInt(path, "server", "tcp_port", cfg.tcpPort);
    cfg.udpPort = ini::ReadInt(path, "server", "udp_port", cfg.udpPort);
    cfg.redisHost = ini::ReadValue(path, "server", "redis_host", cfg.redisHost);
    cfg.redisPort = ini::ReadInt(path, "server", "redis_port", cfg.redisPort);
    cfg.kafkaBrokers = ini::ReadValue(path, "server", "kafka_brokers", cfg.kafkaBrokers);
    cfg.kafkaTopic = ini::ReadValue(path, "server", "kafka_topic", cfg.kafkaTopic);
    cfg.mysqlHost = ini::ReadValue(path, "server", "mysql_host", cfg.mysqlHost);
    cfg.mysqlPort = ini::ReadInt(path, "server", "mysql_port", cfg.mysqlPort);
    cfg.mysqlDb = ini::ReadValue(path, "server", "mysql_db", cfg.mysqlDb);
    cfg.mysqlUser = ini::ReadValue(path, "server", "mysql_user", cfg.mysqlUser);
    cfg.mysqlPassword = ini::ReadValue(path, "server", "mysql_password", cfg.mysqlPassword);
    cfg.logFile = ini::ReadValue(path, "server", "log_file", cfg.logFile);
    cfg.heartbeatIntervalMs = ini::ReadInt(path, "server", "heartbeat_interval_ms", cfg.heartbeatIntervalMs);
    cfg.inactivityTimeoutMs = ini::ReadInt(path, "server", "inactivity_timeout_ms", cfg.inactivityTimeoutMs);
    cfg.cacheTTLSeconds = ini::ReadInt(path, "server", "cache_ttl_seconds", cfg.cacheTTLSeconds);
    return cfg;
}
