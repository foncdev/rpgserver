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

class GatewayServer {
public:
    GatewayServer() {
        if (!Common::GatewayServerConfig::LoadConfig()) {
            LOG_WARNING("GATEWAY", "Failed to load config, using defaults");
        }

        port_ = Common::GatewayServerConfig::GetPort();
        max_connections_ = Common::GatewayServerConfig::GetMaxConnections();
        log_level_ = Common::GatewayServerConfig::GetLogLevel();
    }

    bool Initialize() {
        Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
        Common::LogManager::Instance().SetConsoleOutput(true);
        Common::LogManager::Instance().SetFileOutput(true, "logs/gateway_server.log");

        LOG_INFO("GATEWAY", "Initializing Gateway Server...");
        LOG_INFO_FORMAT("GATEWAY", "Port: %d, Max Connections: %d", port_, max_connections_);
        LOG_INFO_FORMAT("GATEWAY", "Load Balance Method: %s",
                       Common::GatewayServerConfig::GetLoadBalanceMethod().c_str());

        if (!network_manager_.InitializeServer(port_, max_connections_)) {
            LOG_ERROR_FORMAT("GATEWAY", "Failed to initialize Gateway Server on port %d", port_);
            return false;
        }

        SetupCallbacks();
        LOG_INFO("GATEWAY", "Gateway Server initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO_FORMAT("GATEWAY", "Starting Gateway Server on port %d", port_);
        network_manager_.StartServer();

        LOG_INFO("GATEWAY", "Server is running. Commands: status, config, reload, quit");
        ProcessCommands();

        network_manager_.StopServer();
    }

private:
    void SetupCallbacks() {
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("GATEWAY", "Client connected: %s", conn->GetAddress().c_str());
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("GATEWAY", "Client disconnected: %s", conn->GetAddress().c_str());
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });
    }

    void ProcessCommands() {
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                LOG_INFO("GATEWAY", "Shutdown requested by user");
                break;
            } else if (input == "status") {
                PrintStatus();
            } else if (input == "config") {
                PrintConfig();
            } else if (input == "reload") {
                ReloadConfig();
            } else if (input == "help") {
                PrintHelp();
            }
        }
    }

    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                std::string message = "GATEWAY_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                break;
            }
            case Network::PACKET_LOGIN_REQUEST: {
                std::string login_response = "LOGIN_SUCCESS";
                auto response_data = Network::SerializeString(login_response);
                Network::Packet response(Network::PACKET_LOGIN_RESPONSE, response_data);
                network_manager_.SendToClient(conn, response);
                LOG_INFO_FORMAT("GATEWAY", "Login request from %s", conn->GetAddress().c_str());
                break;
            }
            default:
                LOG_WARNING_FORMAT("GATEWAY", "Unknown packet type: %d", packet.type);
                break;
        }
    }

    void PrintConfig() {
        LOG_INFO("GATEWAY", "=== Gateway Server Configuration ===");
        LOG_INFO_FORMAT("GATEWAY", "Port: %d", Common::GatewayServerConfig::GetPort());
        LOG_INFO_FORMAT("GATEWAY", "Max Connections: %d", Common::GatewayServerConfig::GetMaxConnections());
        LOG_INFO_FORMAT("GATEWAY", "Load Balance Method: %s",
                       Common::GatewayServerConfig::GetLoadBalanceMethod().c_str());
        // 추가 설정 출력...
    }

    void ReloadConfig() {
        LOG_INFO("GATEWAY", "Reloading configuration...");
        if (Common::GatewayServerConfig::LoadConfig()) {
            LOG_INFO("GATEWAY", "Configuration reloaded successfully");
        } else {
            LOG_ERROR("GATEWAY", "Failed to reload configuration");
        }
    }

    void PrintStatus() {
        LOG_INFO("GATEWAY", "=== Gateway Server Status ===");
        LOG_INFO_FORMAT("GATEWAY", "Current Connections: %d", network_manager_.GetConnectionCount());
        // 추가 상태 정보...
    }

    void PrintHelp() {
        LOG_INFO("GATEWAY", "=== Available Commands ===");
        LOG_INFO("GATEWAY", "status  - Show server status");
        LOG_INFO("GATEWAY", "config  - Show current configuration");
        LOG_INFO("GATEWAY", "reload  - Reload configuration");
        LOG_INFO("GATEWAY", "help    - Show this help");
        LOG_INFO("GATEWAY", "quit    - Shutdown server");
    }

    Network::NetworkManager network_manager_;
    int port_;
    int max_connections_;
    std::string log_level_;
};

int main() {
    try {
        std::filesystem::create_directories("logs");
        std::filesystem::create_directories("config");

        GatewayServer server;
        if (!server.Initialize()) return -1;

        server.Run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}