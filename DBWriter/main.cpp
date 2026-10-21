#include <iostream>
#include <string>

#include "../Common/Logger.h"
#include "DatabaseWriter.h"
#include "KafkaConsumer.h"

int main(int argc, char* argv[]) {
    std::string configPath = (argc > 1) ? argv[1] : "config/dbwriter.ini";

    DbWriterConfig cfg;
    cfg.kafkaBrokers = "127.0.0.1:9092";
    cfg.kafkaTopic = "chat-messages";
    cfg.kafkaGroupId = "dbwriter-group";
    cfg.mysqlHost = "127.0.0.1";
    cfg.mysqlPort = 3306;
    cfg.mysqlDb = "ChatDatabase";
    cfg.mysqlUser = "root";
    cfg.mysqlPassword = "";
    cfg.logFile = "logs/dbwriter.log";

    Logger::Init(cfg.logFile);

    DatabaseWriter writer;
    if (!writer.Connect(cfg)) {
        Logger::Warn("DBWriter  : MySQL not ready; continuing and retrying on message consumption");
    }

    KafkaConsumer consumer;
    if (!consumer.Start(cfg, &writer)) {
        Logger::Error("DBWriter  : Kafka consumer failed to start");
        Logger::Close();
        return 1;
    }

    Logger::Info("DBWriter  : service started");
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "q" || line == "quit") break;
    }

    consumer.Stop();
    writer.Close();
    Logger::Close();
    return 0;
}
