// common/config_manager.h - Updated for Independent Server Configs
#pragma once
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <vector>

namespace Common {

class ConfigManager {
public:
    static ConfigManager& Instance();

    // 설정 파일 로드/저장
    bool LoadFromFile(const std::string& filename);
    bool SaveToFile(const std::string& filename) const;

    // 설정값 읽기
    std::string GetString(const std::string& section, const std::string& key, const std::string& default_value = "") const;
    int GetInt(const std::string& section, const std::string& key, int default_value = 0) const;
    bool GetBool(const std::string& section, const std::string& key, bool default_value = false) const;
    double GetDouble(const std::string& section, const std::string& key, double default_value = 0.0) const;

    // 설정값 쓰기
    void SetString(const std::string& section, const std::string& key, const std::string& value);
    void SetInt(const std::string& section, const std::string& key, int value);
    void SetBool(const std::string& section, const std::string& key, bool value);
    void SetDouble(const std::string& section, const std::string& key, double value);

    // 섹션/키 존재 확인
    bool HasSection(const std::string& section) const;
    bool HasKey(const std::string& section, const std::string& key) const;

    // 섹션 및 키 목록 가져오기
    std::vector<std::string> GetSections() const;
    std::vector<std::string> GetKeys(const std::string& section) const;

    // 설정 삭제
    void RemoveSection(const std::string& section);
    void RemoveKey(const std::string& section, const std::string& key);

    // 모든 설정 초기화
    void Clear();

    // 기본 설정 생성 (서버 타입별)
    void LoadDefaultConfig(const std::string& server_type);

public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string TrimString(const std::string& str) const;
    std::string CreateKey(const std::string& section, const std::string& key) const;

    mutable std::mutex config_mutex_;
    std::map<std::string, std::string> config_data_;
};

// 서버별 설정 헬퍼 클래스들
class AuthServerConfig {
public:
    static bool LoadConfig(const std::string& config_file = "config/auth_server.conf");
    static bool SaveDefaultConfig(const std::string& config_file = "config/auth_server.conf");

    // Auth Server 전용 설정
    static int GetPort();
    static int GetMaxConnections();
    static std::string GetLogLevel();
    static std::string GetLogFile();
    static bool GetConsoleOutput();
    static bool GetFileOutput();

    // Database 설정 (Auth 서버용)
    static std::string GetDatabaseHost();
    static int GetDatabasePort();
    static std::string GetDatabaseName();
    static std::string GetDatabaseUser();
    static std::string GetDatabasePassword();
    static int GetConnectionPoolSize();

    // Security 설정
    static std::string GetJwtSecret();
    static int GetJwtExpirationHours();
    static int GetPasswordHashRounds();
    static bool GetSslEnabled();

private:
    static ConfigManager& GetConfig();
    static void LoadDefaults();
};

class GatewayServerConfig {
public:
    static bool LoadConfig(const std::string& config_file = "config/gateway_server.conf");
    static bool SaveDefaultConfig(const std::string& config_file = "config/gateway_server.conf");

    // Gateway Server 전용 설정
    static int GetPort();
    static int GetMaxConnections();
    static std::string GetLogLevel();
    static std::string GetLogFile();
    static bool GetConsoleOutput();
    static bool GetFileOutput();

    // Load Balancing 설정
    static std::string GetLoadBalanceMethod(); // round_robin, least_connections, weighted
    static int GetHealthCheckInterval();
    static int GetConnectionTimeout();

    // Upstream 서버들 설정
    static std::vector<std::string> GetAuthServers();
    static std::vector<std::string> GetGameServers();
    static int GetMaxRetries();
    static int GetRetryDelay();

    // Rate Limiting 설정
    static bool GetRateLimitEnabled();
    static int GetRateLimitRequests();
    static int GetRateLimitWindow();

private:
    static ConfigManager& GetConfig();
    static void LoadDefaults();
};

class GameServerConfig {
public:
    static bool LoadConfig(const std::string& config_file = "config/game_server.conf");
    static bool SaveDefaultConfig(const std::string& config_file = "config/game_server.conf");

    // Game Server 전용 설정
    static int GetPort();
    static int GetMaxConnections();
    static int GetTickRate();
    static std::string GetLogLevel();
    static std::string GetLogFile();
    static bool GetConsoleOutput();
    static bool GetFileOutput();

    // Game Logic 설정
    static int GetMaxPlayersPerZone();
    static double GetPlayerMoveSpeed();
    static int GetViewDistance();
    static bool GetPvpEnabled();
    static int GetSaveInterval();

    // Performance 설정
    static int GetWorkerThreads();
    static int GetUpdateQueueSize();
    static bool GetOptimizedNetworking();
    static int GetBatchSize();

    // Zone Server 연결 설정
    static std::vector<std::string> GetZoneServers();
    static int GetZoneConnectionTimeout();

private:
    static ConfigManager& GetConfig();
    static void LoadDefaults();
};

class ZoneServerConfig {
public:
    static bool LoadConfig(const std::string& config_file = "config/zone_server.conf");
    static bool SaveDefaultConfig(const std::string& config_file = "config/zone_server.conf");

    // Zone Server 전용 설정
    static int GetPort();
    static int GetMaxConnections();
    static int GetZoneId();
    static std::string GetLogLevel();
    static std::string GetLogFile();
    static bool GetConsoleOutput();
    static bool GetFileOutput();

    // Map 설정
    static int GetMapWidth();
    static int GetMapHeight();
    static std::string GetMapFile();
    static bool GetMapValidationEnabled();

    // NPC 설정
    static int GetMaxNpcs();
    static int GetNpcSpawnInterval();
    static std::string GetNpcDataFile();

    // Instance 설정
    static bool GetInstanceEnabled();
    static int GetMaxInstances();
    static int GetInstanceTimeout();

    // Physics 설정
    static double GetPhysicsTickRate();
    static bool GetCollisionEnabled();
    static double GetGravity();

private:
    static ConfigManager& GetConfig();
    static void LoadDefaults();
};

} // namespace Common