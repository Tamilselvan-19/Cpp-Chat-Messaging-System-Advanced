#pragma once

#include <map>
#include <string>
#include <vector>

std::string JsonEscape(const std::string& value);
std::string JsonBuild(const std::vector<std::pair<std::string, std::string>>& fields);
bool JsonParse(const std::string& json, std::map<std::string, std::string>& out);
std::string JsonGet(const std::map<std::string, std::string>& obj,
                    const std::string& key,
                    const std::string& defaultValue = "");
