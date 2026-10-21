#include <string>
#include <fstream>
#include <windows.h>

namespace ini {
    static std::string ReadValue(const std::string& path, const std::string& section, const std::string& key,
                                const std::string& defaultValue = "") {
        char buffer[4096] = {};
        std::string result = defaultValue;
        std::string currentSection = "";
        std::ifstream in(path);
        if (!in) return result;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
                continue;
            }
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string k = line.substr(0, pos);
            std::string v = line.substr(pos + 1);
            if (!k.empty() && !v.empty()) {
                if (currentSection == section && k == key) {
                    result = v;
                    break;
                }
            }
        }
        return result;
    }

    static int ReadInt(const std::string& path, const std::string& section, const std::string& key, int defaultValue = 0) {
        std::string val = ReadValue(path, section, key, std::to_string(defaultValue));
        try { return std::stoi(val); } catch (...) { return defaultValue; }
    }
}
