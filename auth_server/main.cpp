#include "../network/network_manager.h"
#include "../common/log_manager.h"
#include "../common/config_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

// 로그 레벨 문자열을 enum으로 변환하는 헬퍼 함수
Common::LogLevel StringToLogLevel(const std::string& level) {
    if (level == "DEBUG") return Common::LogLevel::DEBUG;
    if (level == "INFO") return Common::LogLevel::INFO;
    if (level == "WARNING") return Common::LogLevel::WARNING;
    if (level == "ERROR") return Common::LogLevel::ERROR;
    if (level == "CRITICAL") return Common::LogLevel::CRITICAL;
    return Common::LogLevel::INFO;
}

class AuthServer {
public:
    AuthServer() {
        // 설정 로드
        if (!Common::AuthServerConfig::LoadConfig()) {
            LOG_WARNING("AUTH", "Failed to load config, using defaults");
        }

        port_ = Common::AuthServerConfig::GetPort();
        max_connections_ = Common::AuthServerConfig::GetMaxConnections();
        log_level_ = Common::AuthServerConfig::GetLogLevel();
        log_file_ = Common::AuthServerConfig::GetLogFile();
    }

    bool Initialize() {
        // 로그 매니저 초기화
        Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
        Common::LogManager::Instance().SetConsoleOutput(Common::AuthServerConfig::GetConsoleOutput());
        Common::LogManager::Instance().SetFileOutput(Common::AuthServerConfig::GetFileOutput(), log_file_);

        LOG_INFO("AUTH", "Initializing Authentication Server...");
        LOG_INFO_FORMAT("AUTH", "Port: %d, Max Connections: %d, Log Level: %s",
                       port_, max_connections_, log_level_.c_str());
        LOG_INFO_FORMAT("AUTH", "JWT Secret Length: %zu, Database: %s@%s:%d",
                       Common::AuthServerConfig::GetJwtSecret().length(),
                       Common::AuthServerConfig::GetDatabaseName().c_str(),
                       Common::AuthServerConfig::GetDatabaseHost().c_str(),
                       Common::AuthServerConfig::GetDatabasePort());

        if (!network_manager_.InitializeServer(port_, max_connections_)) {
            LOG_ERROR_FORMAT("AUTH", "Failed to initialize Auth Server on port %d", port_);
            return false;
        }

        SetupCallbacks();
        LOG_INFO("AUTH", "Authentication Server initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO_FORMAT("AUTH", "Starting Authentication Server on port %d", port_);
        network_manager_.StartServer();

        LOG_INFO("AUTH", "Server is running. Commands: status, config, reload, quit");
        ProcessCommands();

        LOG_INFO("AUTH", "Stopping Authentication Server...");
        network_manager_.StopServer();
        LOG_INFO("AUTH", "Authentication Server stopped");
    }

private:
    void SetupCallbacks() {
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("AUTH", "Client connected: %s (ID: %d)",
                           conn->GetAddress().c_str(), conn->GetId());
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("AUTH", "Client disconnected: %s (ID: %d)",
                           conn->GetAddress().c_str(), conn->GetId());
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });
    }

    void ProcessCommands() {
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                LOG_INFO("AUTH", "Shutdown requested by user");
                break;
            } else if (input == "status") {
                PrintStatus();
            } else if (input == "config") {
                PrintConfig();
            } else if (input == "reload") {
                ReloadConfig();
            } else if (input == "help") {
                PrintHelp();
            } else if (!input.empty()) {
                LOG_WARNING_FORMAT("AUTH", "Unknown command: %s", input.c_str());
            }
        }
    }

    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        LOG_DEBUG_FORMAT("AUTH", "Received packet type %d from %s",
                        packet.type, conn->GetAddress().c_str());

        switch (packet.type) {
            case Network::PACKET_ECHO: {
                std::string message = "AUTH_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                break;
            }
            case Network::PACKET_AUTH_REQUEST: {
                // 실제 인증 로직
                std::string auth_response = "AUTH_SUCCESS";
                auto response_data = Network::SerializeString(auth_response);
                Network::Packet response(Network::PACKET_AUTH_RESPONSE, response_data);
                network_manager_.SendToClient(conn, response);
                LOG_INFO_FORMAT("AUTH", "Authentication processed for %s", conn->GetAddress().c_str());
                break;
            }
            default:
                LOG_WARNING_FORMAT("AUTH", "Unknown packet type %d from %s",
                                 packet.type, conn->GetAddress().c_str());
                break;
        }
    }

    void PrintStatus() {
        int connection_count = network_manager_.GetConnectionCount();
        LOG_INFO("AUTH", "=== Authentication Server Status ===");
        LOG_INFO_FORMAT("AUTH", "Port: %d", port_);
        LOG_INFO_FORMAT("AUTH", "Max Connections: %d", max_connections_);
        LOG_INFO_FORMAT("AUTH", "Current Connections: %d", connection_count);
        LOG_INFO_FORMAT("AUTH", "Log Level: %s", log_level_.c_str());
        LOG_INFO_FORMAT("AUTH", "Server Running: %s", network_manager_.IsServerRunning() ? "Yes" : "No");
    }

    void PrintConfig() {
        LOG_INFO("AUTH", "=== Authentication Server Configuration ===");
        LOG_INFO_FORMAT("AUTH", "Port: %d", Common::AuthServerConfig::GetPort());
        LOG_INFO_FORMAT("AUTH", "Max Connections: %d", Common::AuthServerConfig::GetMaxConnections());
        LOG_INFO_FORMAT("AUTH", "Log Level: %s", Common::AuthServerConfig::GetLogLevel().c_str());
        LOG_INFO_FORMAT("AUTH", "Log File: %s", Common::AuthServerConfig::GetLogFile().c_str());
        LOG_INFO_FORMAT("AUTH", "Database Host: %s", Common::AuthServerConfig::GetDatabaseHost().c_str());
        LOG_INFO_FORMAT("AUTH", "Database Port: %d", Common::AuthServerConfig::GetDatabasePort());
        LOG_INFO_FORMAT("AUTH", "Database Name: %s", Common::AuthServerConfig::GetDatabaseName().c_str());
        LOG_INFO_FORMAT("AUTH", "JWT Expiration: %d hours", Common::AuthServerConfig::GetJwtExpirationHours());
    }

    void ReloadConfig() {
        LOG_INFO("AUTH", "Reloading configuration...");

        if (Common::AuthServerConfig::LoadConfig()) {
            // 런타임에 변경 가능한 설정들 적용
            std::string new_log_level = Common::AuthServerConfig::GetLogLevel();
            if (new_log_level != log_level_) {
                log_level_ = new_log_level;
                Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
                LOG_INFO_FORMAT("AUTH", "Log level changed to: %s", log_level_.c_str());
            }

            LOG_INFO("AUTH", "Configuration reloaded successfully");
        } else {
            LOG_ERROR("AUTH", "Failed to reload configuration");
        }
    }

    void PrintHelp() {
        LOG_INFO("AUTH", "=== Available Commands ===");
        LOG_INFO("AUTH", "status  - Show server status");
        LOG_INFO("AUTH", "config  - Show current configuration");
        LOG_INFO("AUTH", "reload  - Reload configuration from file");
        LOG_INFO("AUTH", "help    - Show this help");
        LOG_INFO("AUTH", "quit    - Shutdown server");
    }

    Network::NetworkManager network_manager_;
    int port_;
    int max_connections_;
    std::string log_level_;
    std::string log_file_;
};

int main() {
    try {
        // 로그 디렉토리 생성
        std::filesystem::create_directories("logs");
        std::filesystem::create_directories("config");

        AuthServer server;

        if (!server.Initialize()) {
            return -1;
        }

        server.Run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}