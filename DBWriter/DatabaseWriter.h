#pragma once

#include <mutex>
#include <string>

#include "KafkaConsumer.h"

class DatabaseWriter {
public:
    bool Connect(const DbWriterConfig& config);
    bool EnsureConnected(const DbWriterConfig& config);
    bool InsertMessage(const std::string& messageId,
                       const std::string& userId,
                       const std::string& timestamp,
                       const std::string& message);
    bool IsConnected() const { return m_connected; }
    void Close();

private:
    std::mutex m_mutex;
    void* m_connRaw = nullptr;
    void* m_envRaw = nullptr;
    bool m_connected = false;
    DbWriterConfig m_cfg;
};
