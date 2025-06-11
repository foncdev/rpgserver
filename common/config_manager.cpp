// common/config_manager.cpp - Complete Implementation
#include "config_manager.h"
#include "log_manager.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace Common {

// ConfigManager 기본 구현
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

// 기본 메서드들
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

double ConfigManager::GetDouble(const std::string& section, const std::string& key, double default_value) const {
    std::string value = GetString(section, key);
    if (value.empty()) return default_value;

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
    for (const auto& [key, value] : config_data_) {
        if (key.substr(0, key.find('.')) == section) {
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

    for (const auto& [full_key, value] : config_data_) {
        size_t dot_pos = full_key.find('.');
        if (dot_pos != std::string::npos) {
            std::string key_section = full_key.substr(0, dot_pos);
            if (key_section == section) {
                std::string key = full_key.substr(dot_pos + 1);
                keys.push_back(key);
            }
        }
    }

    return keys;
}

void ConfigManager::RemoveSection(const std::string& section) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = config_data_.begin();
    while (it != config_data_.end()) {
        if (it->first.substr(0, it->first.find('.')) == section) {
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

bool AuthServerConfig::GetConsoleOutput() {
    return GetConfig().GetBool("server", "console_output", true);
}

bool AuthServerConfig::GetFileOutput() {
    return GetConfig().GetBool("server", "file_output", true);
}

std::string AuthServerConfig::GetDatabaseHost() {
    return GetConfig().GetString("database", "host", "localhost");
}

int AuthServerConfig::GetDatabasePort() {
    return GetConfig().GetInt("database", "port", 3306);
}

std::string AuthServerConfig::GetDatabaseName() {
    return GetConfig().GetString("database", "name", "mmorpg_auth");
}

std::string AuthServerConfig::GetDatabaseUser() {
    return GetConfig().GetString("database", "user", "auth_user");
}

std::string AuthServerConfig::GetDatabasePassword() {
    return GetConfig().GetString("database", "password", "auth_password");
}

int AuthServerConfig::GetConnectionPoolSize() {
    return GetConfig().GetInt("database", "connection_pool_size", 10);
}

std::string AuthServerConfig::GetJwtSecret() {
    return GetConfig().GetString("security", "jwt_secret", "default-secret");
}

int AuthServerConfig::GetJwtExpirationHours() {
    return GetConfig().GetInt("security", "jwt_expiration_hours", 24);
}

int AuthServerConfig::GetPasswordHashRounds() {
    return GetConfig().GetInt("security", "password_hash_rounds", 12);
}

bool AuthServerConfig::GetSslEnabled() {
    return GetConfig().GetBool("security", "ssl_enabled", false);
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

bool GatewayServerConfig::SaveDefaultConfig(const std::string& config_file) {
    LoadDefaults();
    return GetConfig().SaveToFile(config_file);
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

int GatewayServerConfig::GetMaxConnections() {
    return GetConfig().GetInt("server", "max_connections", 5000);
}

std::string GatewayServerConfig::GetLogLevel() {
    return GetConfig().GetString("server", "log_level", "INFO");
}

std::string GatewayServerConfig::GetLogFile() {
    return GetConfig().GetString("server", "log_file", "logs/gateway_server.log");
}

bool GatewayServerConfig::GetConsoleOutput() {
    return GetConfig().GetBool("server", "console_output", true);
}

bool GatewayServerConfig::GetFileOutput() {
    return GetConfig().GetBool("server", "file_output", true);
}

std::string GatewayServerConfig::GetLoadBalanceMethod() {
    return GetConfig().GetString("load_balance", "method", "round_robin");
}

int GatewayServerConfig::GetHealthCheckInterval() {
    return GetConfig().GetInt("load_balance", "health_check_interval", 30);
}

int GatewayServerConfig::GetConnectionTimeout() {
    return GetConfig().GetInt("load_balance", "connection_timeout", 5000);
}

std::vector<std::string> GatewayServerConfig::GetAuthServers() {
    std::string servers = GetConfig().GetString("upstream", "auth_servers", "localhost:8001");
    std::vector<std::string> result;
    // 간단한 구현 - 실제로는 콤마로 분리해야 함
    result.push_back(servers);
    return result;
}

std::vector<std::string> GatewayServerConfig::GetGameServers() {
    std::string servers = GetConfig().GetString("upstream", "game_servers", "localhost:8003");
    std::vector<std::string> result;
    result.push_back(servers);
    return result;
}

int GatewayServerConfig::GetMaxRetries() {
    return GetConfig().GetInt("load_balance", "max_retries", 3);
}

int GatewayServerConfig::GetRetryDelay() {
    return GetConfig().GetInt("load_balance", "retry_delay", 1000);
}

bool GatewayServerConfig::GetRateLimitEnabled() {
    return GetConfig().GetBool("rate_limit", "enabled", true);
}

int GatewayServerConfig::GetRateLimitRequests() {
    return GetConfig().GetInt("rate_limit", "requests", 100);
}

int GatewayServerConfig::GetRateLimitWindow() {
    return GetConfig().GetInt("rate_limit", "window", 60);
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

bool GameServerConfig::SaveDefaultConfig(const std::string& config_file) {
    LoadDefaults();
    return GetConfig().SaveToFile(config_file);
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
    config.SetDouble("game", "player_move_speed", 5.0);
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

int GameServerConfig::GetMaxConnections() {
    return GetConfig().GetInt("server", "max_connections", 2000);
}

int GameServerConfig::GetTickRate() {
    return GetConfig().GetInt("server", "tick_rate", 20);
}

std::string GameServerConfig::GetLogLevel() {
    return GetConfig().GetString("server", "log_level", "INFO");
}

std::string GameServerConfig::GetLogFile() {
    return GetConfig().GetString("server", "log_file", "logs/game_server.log");
}

bool GameServerConfig::GetConsoleOutput() {
    return GetConfig().GetBool("server", "console_output", true);
}

bool GameServerConfig::GetFileOutput() {
    return GetConfig().GetBool("server", "file_output", true);
}

int GameServerConfig::GetMaxPlayersPerZone() {
    return GetConfig().GetInt("game", "max_players_per_zone", 100);
}

double GameServerConfig::GetPlayerMoveSpeed() {
    return GetConfig().GetDouble("game", "player_move_speed", 5.0);
}

int GameServerConfig::GetViewDistance() {
    return GetConfig().GetInt("game", "view_distance", 50);
}

bool GameServerConfig::GetPvpEnabled() {
    return GetConfig().GetBool("game", "pvp_enabled", true);
}

int GameServerConfig::GetSaveInterval() {
    return GetConfig().GetInt("game", "save_interval", 300);
}

int GameServerConfig::GetWorkerThreads() {
    return GetConfig().GetInt("performance", "worker_threads", 4);
}

int GameServerConfig::GetUpdateQueueSize() {
    return GetConfig().GetInt("performance", "update_queue_size", 1000);
}

bool GameServerConfig::GetOptimizedNetworking() {
    return GetConfig().GetBool("performance", "optimized_networking", true);
}

int GameServerConfig::GetBatchSize() {
    return GetConfig().GetInt("performance", "batch_size", 10);
}

std::vector<std::string> GameServerConfig::GetZoneServers() {
    std::string servers = GetConfig().GetString("zones", "servers", "localhost:8004");
    std::vector<std::string> result;
    result.push_back(servers);
    return result;
}

int GameServerConfig::GetZoneConnectionTimeout() {
    return GetConfig().GetInt("zones", "connection_timeout", 5000);
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

bool ZoneServerConfig::SaveDefaultConfig(const std::string& config_file) {
    LoadDefaults();
    return GetConfig().SaveToFile(config_file);
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
    config.SetDouble("physics", "tick_rate", 60.0);
    config.SetBool("physics", "collision_enabled", true);
    config.SetDouble("physics", "gravity", 9.81);
}

int ZoneServerConfig::GetPort() {
    return GetConfig().GetInt("server", "port", 8004);
}

int ZoneServerConfig::GetMaxConnections() {
    return GetConfig().GetInt("server", "max_connections", 1000);
}

int ZoneServerConfig::GetZoneId() {
    return GetConfig().GetInt("server", "zone_id", 1);
}

std::string ZoneServerConfig::GetLogLevel() {
    return GetConfig().GetString("server", "log_level", "INFO");
}

std::string ZoneServerConfig::GetLogFile() {
    return GetConfig().GetString("server", "log_file", "logs/zone_server.log");
}

bool ZoneServerConfig::GetConsoleOutput() {
    return GetConfig().GetBool("server", "console_output", true);
}

bool ZoneServerConfig::GetFileOutput() {
    return GetConfig().GetBool("server", "file_output", true);
}

int ZoneServerConfig::GetMapWidth() {
    return GetConfig().GetInt("map", "width", 100);
}

int ZoneServerConfig::GetMapHeight() {
    return GetConfig().GetInt("map", "height", 100);
}

std::string ZoneServerConfig::GetMapFile() {
    return GetConfig().GetString("map", "file", "maps/zone_1.map");
}

bool ZoneServerConfig::GetMapValidationEnabled() {
    return GetConfig().GetBool("map", "validation_enabled", true);
}

int ZoneServerConfig::GetMaxNpcs() {
    return GetConfig().GetInt("npc", "max_npcs", 200);
}

int ZoneServerConfig::GetNpcSpawnInterval() {
    return GetConfig().GetInt("npc", "spawn_interval", 5);
}

std::string ZoneServerConfig::GetNpcDataFile() {
    return GetConfig().GetString("npc", "data_file", "data/npcs.json");
}

bool ZoneServerConfig::GetInstanceEnabled() {
    return GetConfig().GetBool("instance", "enabled", false);
}

int ZoneServerConfig::GetMaxInstances() {
    return GetConfig().GetInt("instance", "max_instances", 10);
}

int ZoneServerConfig::GetInstanceTimeout() {
    return GetConfig().GetInt("instance", "timeout", 3600);
}

double ZoneServerConfig::GetPhysicsTickRate() {
    return GetConfig().GetDouble("physics", "tick_rate", 60.0);
}

bool ZoneServerConfig::GetCollisionEnabled() {
    return GetConfig().GetBool("physics", "collision_enabled", true);
}

double ZoneServerConfig::GetGravity() {
    return GetConfig().GetDouble("physics", "gravity", 9.81);
}

} // namespace Common