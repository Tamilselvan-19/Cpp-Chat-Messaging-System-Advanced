#pragma once

#include <string>
#include <vector>

struct DbWriterConfig {
    std::string kafkaBrokers = "127.0.0.1:9092";
    std::string kafkaTopic = "chat-messages";
    std::string kafkaGroupId = "dbwriter-group";
    std::string mysqlHost = "127.0.0.1";
    int mysqlPort = 3306;
    std::string mysqlDb = "ChatDatabase";
    std::string mysqlUser = "root";
    std::string mysqlPassword = "";
    std::string logFile = "logs/dbwriter.log";
};

class DatabaseWriter;

class KafkaConsumer {
public:
    bool Start(const DbWriterConfig& cfg, DatabaseWriter* writer);
    void Stop();

private:
    void PollLoop();
    DbWriterConfig m_cfg;
    DatabaseWriter* m_writer = nullptr;
    void* m_consumer = nullptr;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};
