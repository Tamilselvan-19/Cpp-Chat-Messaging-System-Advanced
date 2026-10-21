#include "Logger.h"

#include <fstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <sstream>

namespace {
    std::mutex g_log_mutex;
    std::ofstream g_log_file;
}

void Logger::Init(const std::string& file) {
    g_log_file.open(file, std::ios::out | std::ios::app);
}

void Logger::Info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[INFO] " << msg << std::endl;
    if (g_log_file.is_open()) {
        g_log_file << "[INFO] " << msg << std::endl;
        g_log_file.flush();
    }
}

void Logger::Warn(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[WARN] " << msg << std::endl;
    if (g_log_file.is_open()) {
        g_log_file << "[WARN] " << msg << std::endl;
        g_log_file.flush();
    }
}

void Logger::Error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[ERROR] " << msg << std::endl;
    if (g_log_file.is_open()) {
        g_log_file << "[ERROR] " << msg << std::endl;
        g_log_file.flush();
    }
}

void Logger::Close() {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_log_file.is_open()) {
        g_log_file.close();
    }
}
