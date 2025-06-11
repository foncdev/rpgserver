// common/config_manager.h
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

    // 기본 설정 로드
    void LoadDefaultConfig();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string TrimString(const std::string& str) const;
    std::string CreateKey(const std::string& section, const std::string& key) const;

    mutable std::mutex config_mutex_;
    std::map<std::string, std::string> config_data_;
};

// 자주 사용하는 설정들을 위한 헬퍼 클래스
class ServerConfig {
public:
    static void InitializeDefaults();

    // Auth Server 설정
    static int GetAuthServerPort();
    static int GetAuthServerMaxConnections();
    static std::string GetAuthServerLogLevel();

    // Gateway Server 설정
    static int GetGatewayServerPort();
    static int GetGatewayServerMaxConnections();
    static std::string GetGatewayServerLogLevel();

    // Game Server 설정
    static int GetGameServerPort();
    static int GetGameServerMaxConnections();
    static int GetGameServerTickRate();
    static std::string GetGameServerLogLevel();

    // Zone Server 설정
    static int GetZoneServerPort();
    static int GetZoneServerMaxConnections();
    static int GetZoneServerZoneId();
    static int GetZoneServerMapWidth();
    static int GetZoneServerMapHeight();
    static std::string GetZoneServerLogLevel();

    // Network 설정
    static int GetNetworkTimeout();
    static int GetNetworkBufferSize();
    static bool GetNetworkKeepAlive();

    // Logging 설정
    static bool GetLogConsoleOutput();
    static bool GetLogFileOutput();
    static std::string GetLogFilename();
    static std::string GetLogLevel();

    // Database 설정 (미래 확장용)
    static std::string GetDatabaseHost();
    static int GetDatabasePort();
    static std::string GetDatabaseName();
    static std::string GetDatabaseUser();
    static std::string GetDatabasePassword();
};

} // namespace Common