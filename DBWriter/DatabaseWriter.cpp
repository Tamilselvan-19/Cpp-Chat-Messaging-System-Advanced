#include "DatabaseWriter.h"

#include <mysql.h>

#include "../Common/Logger.h"

namespace {
    std::string NormalizeTimestamp(std::string ts) {
        for (char& ch : ts) {
            if (ch == 'T') ch = ' ';
            if (ch == 'Z') ch = ' ';
        }
        while (!ts.empty() && ts.back() == ' ') ts.pop_back();
        return ts;
    }
}

bool DatabaseWriter::Connect(const DbWriterConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_cfg = config;
    m_connected = false;

    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        Logger::Error("DBWriter  : mysql_init failed");
        return false;
    }

    if (!mysql_real_connect(conn, config.mysqlHost.c_str(), config.mysqlUser.c_str(),
        config.mysqlPassword.c_str(), config.mysqlDb.c_str(),
        config.mysqlPort, nullptr, 0)) {
        Logger::Error("DBWriter  : connect failed: " + std::string(mysql_error(conn)));
        mysql_close(conn);
        return false;
    }

    m_connRaw = conn;
    m_connected = true;
    Logger::Info("DBWriter  : connected to MySQL");
    return true;
}

bool DatabaseWriter::EnsureConnected(const DbWriterConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected) return true;
    return Connect(config);
}

bool DatabaseWriter::InsertMessage(const std::string& messageId,
                                  const std::string& userId,
                                  const std::string& timestamp,
                                  const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_connected || !m_connRaw) return false;

    std::string normalized = NormalizeTimestamp(timestamp);
    std::string query = "INSERT INTO tbl_ChatDetails (MessageId, UserId, Timestamp, ChatMessage) "
        "VALUES ('" + messageId + "', '" + userId + "', '" + normalized + "', '" + message + "') "
        "ON DUPLICATE KEY UPDATE MessageId = MessageId";

    if (mysql_query((MYSQL*)m_connRaw, query.c_str()) != 0) {
        Logger::Error("DBWriter  : insert failed: " + std::string(mysql_error((MYSQL*)m_connRaw)));
        return false;
    }

    Logger::Info("DBWriter  : stored message " + messageId);
    return true;
}

void DatabaseWriter::Close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connRaw) {
        mysql_close((MYSQL*)m_connRaw);
        m_connRaw = nullptr;
    }
    m_connected = false;
}
