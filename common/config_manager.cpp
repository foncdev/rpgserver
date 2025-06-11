// common/config_manager.cpp - Updated Implementation for Server-Specific Configs
#include "config_manager.h"
#include "log_manager.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace Common {

// ConfigManager 기본 구현 (기존과 동일)
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
            organized_data[""][full_key] = value;
        }
    }

    // 파일에 쓰기
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

// 기본 메서드들 (기존과 동일)
std::string ConfigManager::GetString(const std::string& section, const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    std::string full_key = CreateKey(section, key);
    auto it = config_data_.find(full_key);
    return (it != config_data_.end()) ? it->second : default_value;
}

int ConfigManager::GetInt(const std::string& section, const std::string& key, int default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) return default_value;

    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) return default_value;

    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1" || value == "yes" || value == "on");
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

std::string ConfigManager::TrimString(const std::string& str) const {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string ConfigManager::CreateKey(const std::string& section, const std::string& key) const {
    if (section.empty()) return key;
    return section + "." + key;
}

// ===========================================================================
// AuthServerConfig 구현
// ===========================================================================

ConfigManager& AuthServerConfig::GetConfig() {
    static ConfigManager auth_config;
    return auth_config;
}

bool AuthServerConfig::LoadConfig(const std::string& config_file) {
    if (!GetConfig().LoadFromFile(config_file)) {
        LoadDefaults();
        return SaveDefaultConfig(config_file);
    }
    return true;
}

bool AuthServerConfig::SaveDefaultConfig(const std::string& config_file) {
    LoadDefaults();
    return GetConfig().SaveToFile(config_file);
}

void AuthServerConfig::LoadDefaults() {
    auto& config = GetConfig();
    config.Clear();

    // Server 설정
    config.SetInt("server", "port", 8001);
    config.SetInt("server", "max_connections", 1000);
    config.SetString("server", "log_level", "INFO");
    config.SetString("server", "log_file", "logs/auth_server.log");
    config.SetBool("server", "console_output", true);
    config.SetBool("server", "file_output", true);

    // Database 설정
    config.SetString("database", "host", "localhost");
    config.SetInt("database", "port", 3306);
    config.SetString("database", "name", "mmorpg_auth");
    config.SetString("database", "user", "auth_user");
    config.SetString("database", "password", "auth_password");
    config.SetInt("database", "connection_pool_size", 10);

    // Security 설정
    config.SetString("security", "jwt_secret", "your-super-secret-jwt-key-change-this");
    config.SetInt("security", "jwt_expiration_hours", 24);
    config.SetInt("security", "password_hash_rounds", 12);
    config.SetBool("security", "ssl_enabled", false);
}

int AuthServerConfig::GetPort() {
    return GetConfig().GetInt("server", "port", 8001);
}

int AuthServerConfig::GetMaxConnections() {
    return GetConfig().GetInt("server", "max_connections", 1000);
}

std::string AuthServerConfig::GetLogLevel() {
    return GetConfig().GetString("server", "log_level", "INFO");
}

std::string AuthServerConfig::GetLogFile() {
    return GetConfig().GetString("server", "log_file", "logs/auth_server.log");
}

std::string AuthServerConfig::GetDatabaseHost() {
    return GetConfig().GetString("database", "host", "localhost");
}

std::string AuthServerConfig::GetJwtSecret() {
    return GetConfig().GetString("security", "jwt_secret", "default-secret");
}

// ===========================================================================
// GatewayServerConfig 구현
// ===========================================================================

ConfigManager& GatewayServerConfig::GetConfig() {
    static ConfigManager gateway_config;
    return gateway_config;
}

bool GatewayServerConfig::LoadConfig(const std::string& config_file) {
    if (!GetConfig().LoadFromFile(config_file)) {
        LoadDefaults();
        return SaveDefaultConfig(config_file);
    }
    return true;
}

void GatewayServerConfig::LoadDefaults() {
    auto& config = GetConfig();
    config.Clear();

    // Server 설정
    config.SetInt("server", "port", 8002);
    config.SetInt("server", "max_connections", 5000);
    config.SetString("server", "log_level", "INFO");
    config.SetString("server", "log_file", "logs/gateway_server.log");
    config.SetBool("server", "console_output", true);
    config.SetBool("server", "file_output", true);

    // Load Balancing 설정
    config.SetString("load_balance", "method", "round_robin");
    config.SetInt("load_balance", "health_check_interval", 30);
    config.SetInt("load_balance", "connection_timeout", 5000);
    config.SetInt("load_balance", "max_retries", 3);
    config.SetInt("load_balance", "retry_delay", 1000);

    // Upstream 서버들
    config.SetString("upstream", "auth_servers", "localhost:8001");
    config.SetString("upstream", "game_servers", "localhost:8003");

    // Rate Limiting 설정
    config.SetBool("rate_limit", "enabled", true);
    config.SetInt("rate_limit", "requests", 100);
    config.SetInt("rate_limit", "window", 60);
}

int GatewayServerConfig::GetPort() {
    return GetConfig().GetInt("server", "port", 8002);
}

std::string GatewayServerConfig::GetLoadBalanceMethod() {
    return GetConfig().GetString("load_balance", "method", "round_robin");
}

// ===========================================================================
// GameServerConfig 구현
// ===========================================================================

ConfigManager& GameServerConfig::GetConfig() {
    static ConfigManager game_config;
    return game_config;
}

bool GameServerConfig::LoadConfig(const std::string& config_file) {
    if (!GetConfig().LoadFromFile(config_file)) {
        LoadDefaults();
        return SaveDefaultConfig(config_file);
    }
    return true;
}

void GameServerConfig::LoadDefaults() {
    auto& config = GetConfig();
    config.Clear();

    // Server 설정
    config.SetInt("server", "port", 8003);
    config.SetInt("server", "max_connections", 2000);
    config.SetInt("server", "tick_rate", 20);
    config.SetString("server", "log_level", "INFO");
    config.SetString("server", "log_file", "logs/game_server.log");
    config.SetBool("server", "console_output", true);
    config.SetBool("server", "file_output", true);

    // Game Logic 설정
    config.SetInt("game", "max_players_per_zone", 100);
    config.SetString("game", "player_move_speed", "5.0");
    config.SetInt("game", "view_distance", 50);
    config.SetBool("game", "pvp_enabled", true);
    config.SetInt("game", "save_interval", 300);

    // Performance 설정
    config.SetInt("performance", "worker_threads", 4);
    config.SetInt("performance", "update_queue_size", 1000);
    config.SetBool("performance", "optimized_networking", true);
    config.SetInt("performance", "batch_size", 10);

    // Zone Server 연결
    config.SetString("zones", "servers", "localhost:8004");
    config.SetInt("zones", "connection_timeout", 5000);
}

int GameServerConfig::GetPort() {
    return GetConfig().GetInt("server", "port", 8003);
}

int GameServerConfig::GetTickRate() {
    return GetConfig().GetInt("server", "tick_rate", 20);
}

int GameServerConfig::GetMaxPlayersPerZone() {
    return GetConfig().GetInt("game", "max_players_per_zone", 100);
}

// ===========================================================================
// ZoneServerConfig 구현
// ===========================================================================

ConfigManager& ZoneServerConfig::GetConfig() {
    static ConfigManager zone_config;
    return zone_config;
}

bool ZoneServerConfig::LoadConfig(const std::string& config_file) {
    if (!GetConfig().LoadFromFile(config_file)) {
        LoadDefaults();
        return SaveDefaultConfig(config_file);
    }
    return true;
}

void ZoneServerConfig::LoadDefaults() {
    auto& config = GetConfig();
    config.Clear();

    // Server 설정
    config.SetInt("server", "port", 8004);
    config.SetInt("server", "max_connections", 1000);
    config.SetInt("server", "zone_id", 1);
    config.SetString("server", "log_level", "INFO");
    config.SetString("server", "log_file", "logs/zone_server.log");
    config.SetBool("server", "console_output", true);
    config.SetBool("server", "file_output", true);

    // Map 설정
    config.SetInt("map", "width", 100);
    config.SetInt("map", "height", 100);
    config.SetString("map", "file", "maps/zone_1.map");
    config.SetBool("map", "validation_enabled", true);

    // NPC 설정
    config.SetInt("npc", "max_npcs", 200);
    config.SetInt("npc", "spawn_interval", 5);
    config.SetString("npc", "data_file", "data/npcs.json");

    // Instance 설정
    config.SetBool("instance", "enabled", false);
    config.SetInt("instance", "max_instances", 10);
    config.SetInt("instance", "timeout", 3600);

    // Physics 설정
    config.SetString("physics", "tick_rate", "60.0");
    config.SetBool("physics", "collision_enabled", true);
    config.SetString("physics", "gravity", "9.81");
}

int ZoneServerConfig::GetPort() {
    return GetConfig().GetInt("server", "port", 8004);
}

int ZoneServerConfig::GetZoneId() {
    return GetConfig().GetInt("server", "zone_id", 1);
}

int ZoneServerConfig::GetMapWidth() {
    return GetConfig().GetInt("map", "width", 100);
}

int ZoneServerConfig::GetMapHeight() {
    return GetConfig().GetInt("map", "height", 100);
}

bool ZoneServerConfig::SaveDefaultConfig(const std::string& config_file) {
    LoadDefaults();
    return GetConfig().SaveToFile(config_file);
}

} // namespace Common