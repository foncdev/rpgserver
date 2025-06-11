// auth_server/main.cpp - Updated with Logging and Config
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
    return Common::LogLevel::INFO; // 기본값
}

class AuthServer {
public:
    AuthServer() {
        // 설정 로드
        port_ = Common::ServerConfig::GetAuthServerPort();
        max_connections_ = Common::ServerConfig::GetAuthServerMaxConnections();
        log_level_ = Common::ServerConfig::GetAuthServerLogLevel();
    }

    bool Initialize() {
        // 로그 매니저 초기화
        Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
        Common::LogManager::Instance().SetConsoleOutput(Common::ServerConfig::GetLogConsoleOutput());
        Common::LogManager::Instance().SetFileOutput(
            Common::ServerConfig::GetLogFileOutput(),
            "logs/auth_server.log"
        );

        LOG_INFO("AUTH", "Initializing Authentication Server...");
        LOG_INFO_FORMAT("AUTH", "Port: %d, Max Connections: %d, Log Level: %s",
                       port_, max_connections_, log_level_.c_str());

        if (!network_manager_.InitializeServer(port_, max_connections_)) {
            LOG_ERROR_FORMAT("AUTH", "Failed to initialize Auth Server on port %d", port_);
            return false;
        }

        // 콜백 설정
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

        LOG_INFO("AUTH", "Authentication Server initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO_FORMAT("AUTH", "Starting Authentication Server on port %d", port_);
        network_manager_.StartServer();

        LOG_INFO("AUTH", "Server is running. Commands: status, clients, reload, quit");

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                LOG_INFO("AUTH", "Shutdown requested by user");
                break;
            } else if (input == "status") {
                PrintStatus();
            } else if (input == "clients") {
                PrintClients();
            } else if (input == "reload") {
                ReloadConfig();
            } else if (input == "help") {
                PrintHelp();
            } else if (!input.empty()) {
                LOG_WARNING_FORMAT("AUTH", "Unknown command: %s", input.c_str());
            }
        }

        LOG_INFO("AUTH", "Stopping Authentication Server...");
        network_manager_.StopServer();
        LOG_INFO("AUTH", "Authentication Server stopped");
    }

private:
    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        LOG_DEBUG_FORMAT("AUTH", "Received packet type %d from %s",
                        packet.type, conn->GetAddress().c_str());

        switch (packet.type) {
            case Network::PACKET_ECHO: {
                std::string message = "AUTH_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                LOG_DEBUG_FORMAT("AUTH", "Echo request handled for %s", conn->GetAddress().c_str());
                break;
            }
            case Network::PACKET_AUTH_REQUEST: {
                // 실제 인증 로직이 여기에 들어갈 예정
                std::string auth_response = "AUTH_SUCCESS";
                auto response_data = Network::SerializeString(auth_response);
                Network::Packet response(Network::PACKET_AUTH_RESPONSE, response_data);
                network_manager_.SendToClient(conn, response);
                LOG_INFO_FORMAT("AUTH", "Authentication request processed for %s", conn->GetAddress().c_str());
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

    void PrintClients() {
        auto connections = network_manager_.GetConnections();
        LOG_INFO_FORMAT("AUTH", "=== Connected Clients (%zu) ===", connections.size());
        for (const auto& conn : connections) {
            LOG_INFO_FORMAT("AUTH", "ID: %d, Address: %s",
                           conn->GetId(), conn->GetAddress().c_str());
        }
    }

    void ReloadConfig() {
        LOG_INFO("AUTH", "Reloading configuration...");

        // 설정 파일이 있으면 다시 로드
        if (std::filesystem::exists("config/server.conf")) {
            if (Common::ConfigManager::Instance().LoadFromFile("config/server.conf")) {
                // 새 설정 적용
                int new_log_level_int = static_cast<int>(StringToLogLevel(Common::ServerConfig::GetAuthServerLogLevel()));
                int current_log_level_int = static_cast<int>(StringToLogLevel(log_level_));

                if (new_log_level_int != current_log_level_int) {
                    log_level_ = Common::ServerConfig::GetAuthServerLogLevel();
                    Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
                    LOG_INFO_FORMAT("AUTH", "Log level changed to: %s", log_level_.c_str());
                }

                LOG_INFO("AUTH", "Configuration reloaded successfully");
            } else {
                LOG_ERROR("AUTH", "Failed to reload configuration file");
            }
        } else {
            LOG_WARNING("AUTH", "Configuration file not found, using defaults");
        }
    }

    void PrintHelp() {
        LOG_INFO("AUTH", "=== Available Commands ===");
        LOG_INFO("AUTH", "status  - Show server status");
        LOG_INFO("AUTH", "clients - Show connected clients");
        LOG_INFO("AUTH", "reload  - Reload configuration");
        LOG_INFO("AUTH", "help    - Show this help");
        LOG_INFO("AUTH", "quit    - Shutdown server");
    }

    Network::NetworkManager network_manager_;
    int port_;
    int max_connections_;
    std::string log_level_;
};

int main() {
    try {
        // 설정 초기화
        Common::ServerConfig::InitializeDefaults();

        // 설정 파일이 있으면 로드
        if (std::filesystem::exists("config/server.conf")) {
            if (!Common::ConfigManager::Instance().LoadFromFile("config/server.conf")) {
                std::cerr << "Warning: Failed to load config file, using defaults" << std::endl;
            }
        } else {
            // 기본 설정 파일 생성
            std::filesystem::create_directories("config");
            Common::ConfigManager::Instance().SaveToFile("config/server.conf");
            std::cout << "Created default configuration file: config/server.conf" << std::endl;
        }

        // 로그 디렉토리 생성
        std::filesystem::create_directories("logs");

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