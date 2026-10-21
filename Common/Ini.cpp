#include "Ini.h"

#include <fstream>
#include <sstream>
#include <string>

namespace {
    std::string Trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
}

std::string IniFile::ReadValue(const std::string& path, const std::string& section,
                               const std::string& key, const std::string& defaultValue) {
    std::ifstream in(path);
    if (!in) return defaultValue;

    std::string line;
    std::string currentSection;
    while (std::getline(in, line)) {
        auto trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            currentSection = Trim(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }
        size_t pos = trimmed.find('=');
        if (pos == std::string::npos) continue;
        std::string k = Trim(trimmed.substr(0, pos));
        std::string v = Trim(trimmed.substr(pos + 1));
        if (k == key && currentSection == section) return v;
    }
    return defaultValue;
}

int IniFile::ReadInt(const std::string& path, const std::string& section,
                     const std::string& key, int defaultValue) {
    std::string val = ReadValue(path, section, key, std::to_string(defaultValue));
    try { return std::stoi(val); } catch (...) { return defaultValue; }
}
