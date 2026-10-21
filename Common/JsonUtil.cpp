#include "JsonUtil.h"

#include <string>
#include <map>
#include <vector>

std::string JsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }
    return out;
}

std::string JsonBuild(const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string result = "{";
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) result += ",";
        result += "\"" + JsonEscape(fields[i].first) + "\":" + "\"" + JsonEscape(fields[i].second) + "\"";
    }
    result += "}";
    return result;
}

bool JsonParse(const std::string& json, std::map<std::string, std::string>& out) {
    out.clear();
    std::string s = json;
    if (s.size() < 2 || s.front() != '{' || s.back() != '}') return false;
    s = s.substr(1, s.size() - 2);
    while (!s.empty()) {
        size_t keyStart = s.find('"');
        if (keyStart == std::string::npos) break;
        s = s.substr(keyStart + 1);
        size_t keyEnd = s.find('"');
        if (keyEnd == std::string::npos) return false;
        std::string key = s.substr(0, keyEnd);
        s = s.substr(keyEnd + 2);
        size_t valStart = s.find('"');
        if (valStart == std::string::npos) return false;
        s = s.substr(valStart + 1);
        size_t valEnd = s.find('"');
        if (valEnd == std::string::npos) return false;
        std::string value = s.substr(0, valEnd);
        out[key] = value;
        s = s.substr(valEnd + 1);
        if (!s.empty() && s.front() == ',') s = s.substr(1);
        while (!s.empty() && s.front() == ' ') s = s.substr(1);
    }
    return true;
}

std::string JsonGet(const std::map<std::string, std::string>& obj, const std::string& key,
                   const std::string& defaultValue) {
    auto it = obj.find(key);
    return it == obj.end() ? defaultValue : it->second;
}
