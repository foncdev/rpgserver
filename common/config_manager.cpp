// common/config_manager.cpp
#include "config_manager.h"
#include "log_manager.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace Common {

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string current_section;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        line = TrimString(line);
        
        // 빈 라인이나 주석 건너뛰기
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 섹션 파싱 [section_name]
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            current_section = TrimString(current_section);
            continue;
        }
        
        // 키=값 파싱
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = TrimString(line.substr(0, equals_pos));
            std::string value = TrimString(line.substr(equals_pos + 1));
            
            if (!key.empty()) {
                std::string full_key = CreateKey(current_section, key);
                config_data_[full_key] = value;
            }
        }
    }
    
    file.close();
    return true;
}

bool ConfigManager::SaveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // 디렉토리가 없으면 생성
    std::filesystem::path file_path(filename);
    auto parent_path = file_path.parent_path();
    if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
        try {
            std::filesystem::create_directories(parent_path);
        } catch (const std::exception&) {
            return false;
        }
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // 섹션별로 정리해서 저장
    std::map<std::string, std::map<std::string, std::string>> organized_data;
    
    for (const auto& [full_key, value] : config_data_) {
        size_t dot_pos = full_key.find('.');
        if (dot_pos != std::string::npos) {
            std::string section = full_key.substr(0, dot_pos);
            std::string key = full_key.substr(dot_pos + 1);
            organized_data[section][key] = value;
        } else {
            // 섹션이 없는 키는 빈 섹션에 저장
            organized_data[""][full_key] = value;
        }
    }
    
    // 파일에 쓰기
    file << "# MMORPG Server Configuration File" << std::endl;
    file << "# Generated automatically" << std::endl;
    file << std::endl;
    
    for (const auto& [section, keys] : organized_data) {
        if (!section.empty()) {
            file << "[" << section << "]" << std::endl;
        }
        
        for (const auto& [key, value] : keys) {
            if (!key.empty()) {
                file << key << " = " << value << std::endl;
            }
        }
        
        file << std::endl;
    }
    
    file.close();
    return true;
}

std::string ConfigManager::GetString(const std::string& section, const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string full_key = CreateKey(section, key);
    auto it = config_data_.find(full_key);
    
    return (it != config_data_.end()) ? it->second : default_value;
}

int ConfigManager::GetInt(const std::string& section, const std::string& key, int default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) {
        return default_value;
    }
    
    // 소문자로 변환
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    return (value == "true" || value == "1" || value == "yes" || value == "on");
}

double ConfigManager::GetDouble(const std::string& section, const std::string& key, double default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

void ConfigManager::SetString(const std::string& section, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    std::string full_key = CreateKey(section, key);
    config_data_[full_key] = value;
}

void ConfigManager::SetInt(const std::string& section, const std::string& key, int value) {
    SetString(section, key, std::to_string(value));
}

void ConfigManager::SetBool(const std::string& section, const std::string& key, bool value) {
    SetString(section, key, value ? "true" : "false");
}

void ConfigManager::SetDouble(const std::string& section, const std::string& key, double value) {
    SetString(section, key, std::to_string(value));
}

bool ConfigManager::HasSection(const std::string& section) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string section_prefix = section + ".";
    for (const auto& [key, value] : config_data_) {
        if (key.substr(0, section_prefix.length()) == section_prefix) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::HasKey(const std::string& section, const std::string& key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string full_key = CreateKey(section, key);
    return config_data_.find(full_key) != config_data_.end();
}

std::vector<std::string> ConfigManager::GetSections() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<std::string> sections;
    for (const auto& [key, value] : config_data_) {
        size_t dot_pos = key.find('.');
        if (dot_pos != std::string::npos) {
            std::string section = key.substr(0, dot_pos);
            if (std::find(sections.begin(), sections.end(), section) == sections.end()) {
                sections.push_back(section);
            }
        }
    }
    return sections;
}

std::vector<std::string> ConfigManager::GetKeys(const std::string& section) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::vector<std::string> keys;
    std::string section_prefix = section + ".";
    
    for (const auto& [full_key, value] : config_data_) {
        if (full_key.substr(0, section_prefix.length()) == section_prefix) {
            std::string key = full_key.substr(section_prefix.length());
            keys.push_back(key);
        }
    }
    return keys;
}

void ConfigManager::RemoveSection(const std::string& section) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string section_prefix = section + ".";
    auto it = config_data_.begin();
    while (it != config_data_.end()) {
        if (it->first.substr(0, section_prefix.length()) == section_prefix) {
            it = config_data_.erase(it);
        } else {
            ++it;
        }
    }
}

void ConfigManager::RemoveKey(const std::string& section, const std::string& key) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    std::string full_key = CreateKey(section, key);
    config_data_.erase(full_key);
}

void ConfigManager::Clear() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_data_.clear();
}

void ConfigManager::LoadDefaultConfig() {
    Clear();
    
    // Auth Server 기본 설정
    SetInt("auth_server", "port", 8001);
    SetInt("auth_server", "max_connections", 1000);
    SetString("auth_server", "log_level", "INFO");
    
    // Gateway Server 기본 설정
    SetInt("gateway_server", "port", 8002);
    SetInt("gateway_server", "max_connections", 5000);
    SetString("gateway_server", "log_level", "INFO");
    
    // Game Server 기본 설정
    SetInt("game_server", "port", 8003);
    SetInt("game_server", "max_connections", 2000);
    SetInt("game_server", "tick_rate", 20);
    SetString("game_server", "log_level", "INFO");
    
    // Zone Server 기본 설정
    SetInt("zone_server", "port", 8004);
    SetInt("zone_server", "max_connections", 1000);
    SetInt("zone_server", "zone_id", 1);
    SetInt("zone_server", "map_width", 50);
    SetInt("zone_server", "map_height", 50);
    SetString("zone_server", "log_level", "INFO");
    
    // Network 기본 설정
    SetInt("network", "timeout", 30000);
    SetInt("network", "buffer_size", 8192);
    SetBool("network", "keep_alive", true);
    
    // Logging 기본 설정
    SetBool("logging", "console_output", true);
    SetBool("logging", "file_output", true);
    SetString("logging", "filename", "logs/mmorpg_server.log");
    SetString("logging", "level", "INFO");
    
    // Database 기본 설정 (미래 확장용)
    SetString("database", "host", "localhost");
    SetInt("database", "port", 3306);
    SetString("database", "name", "mmorpg");
    SetString("database", "user", "mmorpg_user");
    SetString("database", "password", "password");
}

std::string ConfigManager::TrimString(const std::string& str) const {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string ConfigManager::CreateKey(const std::string& section, const std::string& key) const {
    if (section.empty()) {
        return key;
    }
    return section + "." + key;
}

// ServerConfig 구현
void ServerConfig::InitializeDefaults() {
    ConfigManager::Instance().LoadDefaultConfig();
}

int ServerConfig::GetAuthServerPort() {
    return ConfigManager::Instance().GetInt("auth_server", "port", 8001);
}

int ServerConfig::GetAuthServerMaxConnections() {
    return ConfigManager::Instance().GetInt("auth_server", "max_connections", 1000);
}

std::string ServerConfig::GetAuthServerLogLevel() {
    return ConfigManager::Instance().GetString("auth_server", "log_level", "INFO");
}

int ServerConfig::GetGatewayServerPort() {
    return ConfigManager::Instance().GetInt("gateway_server", "port", 8002);
}

int ServerConfig::GetGatewayServerMaxConnections() {
    return ConfigManager::Instance().GetInt("gateway_server", "max_connections", 5000);
}

std::string ServerConfig::GetGatewayServerLogLevel() {
    return ConfigManager::Instance().GetString("gateway_server", "log_level", "INFO");
}

int ServerConfig::GetGameServerPort() {
    return ConfigManager::Instance().GetInt("game_server", "port", 8003);
}

int ServerConfig::GetGameServerMaxConnections() {
    return ConfigManager::Instance().GetInt("game_server", "max_connections", 2000);
}

int ServerConfig::GetGameServerTickRate() {
    return ConfigManager::Instance().GetInt("game_server", "tick_rate", 20);
}

std::string ServerConfig::GetGameServerLogLevel() {
    return ConfigManager::Instance().GetString("game_server", "log_level", "INFO");
}

int ServerConfig::GetZoneServerPort() {
    return ConfigManager::Instance().GetInt("zone_server", "port", 8004);
}

int ServerConfig::GetZoneServerMaxConnections() {
    return ConfigManager::Instance().GetInt("zone_server", "max_connections", 1000);
}

int ServerConfig::GetZoneServerZoneId() {
    return ConfigManager::Instance().GetInt("zone_server", "zone_id", 1);
}

int ServerConfig::GetZoneServerMapWidth() {
    return ConfigManager::Instance().GetInt("zone_server", "map_width", 50);
}

int ServerConfig::GetZoneServerMapHeight() {
    return ConfigManager::Instance().GetInt("zone_server", "map_height", 50);
}

std::string ServerConfig::GetZoneServerLogLevel() {
    return ConfigManager::Instance().GetString("zone_server", "log_level", "INFO");
}

int ServerConfig::GetNetworkTimeout() {
    return ConfigManager::Instance().GetInt("network", "timeout", 30000);
}

int ServerConfig::GetNetworkBufferSize() {
    return ConfigManager::Instance().GetInt("network", "buffer_size", 8192);
}

bool ServerConfig::GetNetworkKeepAlive() {
    return ConfigManager::Instance().GetBool("network", "keep_alive", true);
}

bool ServerConfig::GetLogConsoleOutput() {
    return ConfigManager::Instance().GetBool("logging", "console_output", true);
}

bool ServerConfig::GetLogFileOutput() {
    return ConfigManager::Instance().GetBool("logging", "file_output", true);
}

std::string ServerConfig::GetLogFilename() {
    return ConfigManager::Instance().GetString("logging", "filename", "logs/mmorpg_server.log");
}

std::string ServerConfig::GetLogLevel() {
    return ConfigManager::Instance().GetString("logging", "level", "INFO");
}

std::string ServerConfig::GetDatabaseHost() {
    return ConfigManager::Instance().GetString("database", "host", "localhost");
}

int ServerConfig::GetDatabasePort() {
    return ConfigManager::Instance().GetInt("database", "port", 3306);
}

std::string ServerConfig::GetDatabaseName() {
    return ConfigManager::Instance().GetString("database", "name", "mmorpg");
}

std::string ServerConfig::GetDatabaseUser() {
    return ConfigManager::Instance().GetString("database", "user", "mmorpg_user");
}

std::string ServerConfig::GetDatabasePassword() {
    return ConfigManager::Instance().GetString("database", "password", "password");
}

} // namespace Common