// game_server/main.cpp - Updated with Logging and Config
#include "../network/network_manager.h"
#include "../common/log_manager.h"
#include "../common/config_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
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

class GameServer {
public:
    GameServer() {
        // 설정 로드
        if (!Common::GameServerConfig::LoadConfig()) {
            LOG_WARNING("GAME", "Failed to load config, using defaults");
        }

        port_ = Common::GameServerConfig::GetPort();
        max_connections_ = Common::GameServerConfig::GetMaxConnections();
        game_tick_rate_ = Common::GameServerConfig::GetTickRate();
        log_level_ = Common::GameServerConfig::GetLogLevel();
        game_running_ = false;
    }

    bool Initialize() {
        // 로그 매니저 초기화
        Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
        Common::LogManager::Instance().SetConsoleOutput(Common::GameServerConfig::GetConsoleOutput());
        Common::LogManager::Instance().SetFileOutput(
            Common::GameServerConfig::GetFileOutput(),
            Common::GameServerConfig::GetLogFile()
        );

        LOG_INFO("GAME", "Initializing Game Server...");
        LOG_INFO_FORMAT("GAME", "Port: %d, Max Connections: %d, TPS: %d, Log Level: %s",
                       port_, max_connections_, game_tick_rate_, log_level_.c_str());

        if (!network_manager_.InitializeServer(port_, max_connections_)) {
            LOG_ERROR_FORMAT("GAME", "Failed to initialize Game Server on port %d", port_);
            return false;
        }

        // 콜백 설정
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("GAME", "Player connected: %s (ID: %d)",
                           conn->GetAddress().c_str(), conn->GetId());

            // 플레이어 세션 초기화
            std::lock_guard<std::mutex> lock(players_mutex_);
            player_sessions_[conn->GetId()] = {conn->GetId(), conn->GetAddress(), 0, 0};
            LOG_DEBUG_FORMAT("GAME", "Player session created for ID: %d", conn->GetId());
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            LOG_INFO_FORMAT("GAME", "Player disconnected: %s (ID: %d)",
                           conn->GetAddress().c_str(), conn->GetId());

            // 플레이어 세션 제거
            std::lock_guard<std::mutex> lock(players_mutex_);
            player_sessions_.erase(conn->GetId());
            LOG_DEBUG_FORMAT("GAME", "Player session removed for ID: %d", conn->GetId());
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });

        LOG_INFO("GAME", "Game Server initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO_FORMAT("GAME", "Starting Game Server on port %d (TPS: %d)", port_, game_tick_rate_);
        network_manager_.StartServer();

        // 게임 루프 스레드 시작
        game_running_ = true;
        game_thread_ = std::thread(&GameServer::GameLoop, this);
        LOG_INFO("GAME", "Game loop started");

        LOG_INFO("GAME", "Server is running. Commands: status, players, tps <rate>, reload, quit");

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                LOG_INFO("GAME", "Shutdown requested by user");
                break;
            } else if (input == "status") {
                PrintStatus();
            } else if (input == "players") {
                PrintPlayers();
            } else if (input.substr(0, 4) == "tps ") {
                ChangeTPS(input.substr(4));
            } else if (input == "reload") {
                ReloadConfig();
            } else if (input == "help") {
                PrintHelp();
            } else if (!input.empty()) {
                LOG_WARNING_FORMAT("GAME", "Unknown command: %s", input.c_str());
            }
        }

        // 게임 루프 종료
        LOG_INFO("GAME", "Stopping game loop...");
        game_running_ = false;
        if (game_thread_.joinable()) {
            game_thread_.join();
        }
        LOG_INFO("GAME", "Game loop stopped");

        LOG_INFO("GAME", "Stopping Game Server...");
        network_manager_.StopServer();
        LOG_INFO("GAME", "Game Server stopped");
    }

private:
    struct PlayerSession {
        uint32_t player_id;
        std::string address;
        int32_t x, y;
    };

    void GameLoop() {
        LOG_INFO_FORMAT("GAME", "Game loop running at %d TPS", game_tick_rate_);

        const auto tick_duration = std::chrono::milliseconds(1000 / game_tick_rate_);
        auto last_tick = std::chrono::steady_clock::now();
        auto last_stats = std::chrono::steady_clock::now();
        uint64_t tick_count = 0;

        while (game_running_) {
            auto current_time = std::chrono::steady_clock::now();
            auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_tick);

            if (delta_time >= tick_duration) {
                UpdateGame();
                tick_count++;
                last_tick = current_time;
            }

            // 1분마다 통계 출력
            auto stats_delta = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_stats);
            if (stats_delta.count() >= 60) {
                double actual_tps = tick_count / stats_delta.count();
                LOG_DEBUG_FORMAT("GAME", "Game stats - Ticks: %llu, Actual TPS: %.2f",
                               tick_count, actual_tps);
                tick_count = 0;
                last_stats = current_time;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        LOG_INFO("GAME", "Game loop exited");
    }

    void UpdateGame() {
        // 20 TPS 게임 루프 - 현재는 기본적인 플레이어 관리만
        // 실제 게임에서는 플레이어 위치 동기화, 게임 로직 처리 등을 수행

        // 주기적으로 플레이어 상태 동기화 (예시)
        static int sync_counter = 0;
        sync_counter++;

        if (sync_counter >= game_tick_rate_) { // 1초마다
            sync_counter = 0;
            SynchronizePlayers();
        }
    }

    void SynchronizePlayers() {
        std::lock_guard<std::mutex> lock(players_mutex_);
        if (!player_sessions_.empty()) {
            LOG_DEBUG_FORMAT("GAME", "Synchronizing %zu players", player_sessions_.size());
            // 실제 동기화 로직이 여기에 들어갈 예정
        }
    }

    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        LOG_DEBUG_FORMAT("GAME", "Received packet type %d from %s",
                        packet.type, conn->GetAddress().c_str());

        switch (packet.type) {
            case Network::PACKET_ECHO: {
                std::string message = "GAME_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                LOG_DEBUG_FORMAT("GAME", "Echo request handled for %s", conn->GetAddress().c_str());
                break;
            }
            case Network::PACKET_PLAYER_MOVE: {
                HandlePlayerMove(conn, packet);
                break;
            }
            case Network::PACKET_PLAYER_CHAT: {
                HandlePlayerChat(conn, packet);
                break;
            }
            default:
                LOG_WARNING_FORMAT("GAME", "Unknown packet type %d from %s",
                                 packet.type, conn->GetAddress().c_str());
                break;
        }
    }

    void HandlePlayerMove(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        std::lock_guard<std::mutex> lock(players_mutex_);

        if (player_sessions_.find(conn->GetId()) != player_sessions_.end()) {
            // 실제로는 패킷에서 좌표를 파싱해야 함
            auto& player = player_sessions_[conn->GetId()];
            player.x += 1;
            player.y += 1;

            std::string move_response = "MOVE_SUCCESS";
            auto response_data = Network::SerializeString(move_response);
            Network::Packet response(Network::PACKET_PLAYER_MOVE, response_data);
            network_manager_.SendToClient(conn, response);

            LOG_DEBUG_FORMAT("GAME", "Player move: ID %d to (%d, %d)",
                           conn->GetId(), player.x, player.y);
        }
    }

    void HandlePlayerChat(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        // 채팅 메시지 브로드캐스트
        size_t offset = 0;
        std::string chat_message = Network::DeserializeString(packet.data, offset);

        LOG_INFO_FORMAT("GAME", "Chat from %s (ID: %d): %s",
                       conn->GetAddress().c_str(), conn->GetId(), chat_message.c_str());

        std::string broadcast_message = "CHAT_BROADCAST: " + chat_message;
        auto response_data = Network::SerializeString(broadcast_message);
        Network::Packet response(Network::PACKET_PLAYER_CHAT, response_data);
        network_manager_.SendToAll(response);
    }

    void PrintStatus() {
        int connection_count = network_manager_.GetConnectionCount();
        size_t session_count;
        {
            std::lock_guard<std::mutex> lock(players_mutex_);
            session_count = player_sessions_.size();
        }

        LOG_INFO("GAME", "=== Game Server Status ===");
        LOG_INFO_FORMAT("GAME", "Port: %d", port_);
        LOG_INFO_FORMAT("GAME", "Max Connections: %d", max_connections_);
        LOG_INFO_FORMAT("GAME", "Current Connections: %d", connection_count);
        LOG_INFO_FORMAT("GAME", "Active Sessions: %zu", session_count);
        LOG_INFO_FORMAT("GAME", "Target TPS: %d", game_tick_rate_);
        LOG_INFO_FORMAT("GAME", "Log Level: %s", log_level_.c_str());
        LOG_INFO_FORMAT("GAME", "Game Running: %s", game_running_ ? "Yes" : "No");
    }

    void PrintPlayers() {
        std::lock_guard<std::mutex> lock(players_mutex_);
        LOG_INFO_FORMAT("GAME", "=== Active Players (%zu) ===", player_sessions_.size());

        for (const auto& [id, session] : player_sessions_) {
            LOG_INFO_FORMAT("GAME", "ID: %d, Address: %s, Pos: (%d, %d)",
                           id, session.address.c_str(), session.x, session.y);
        }
    }

    void ChangeTPS(const std::string& tps_str) {
        try {
            int new_tps = std::stoi(tps_str);
            if (new_tps >= 1 && new_tps <= 100) {
                game_tick_rate_ = new_tps;
                LOG_INFO_FORMAT("GAME", "TPS changed to: %d", game_tick_rate_);
            } else {
                LOG_WARNING("GAME", "TPS must be between 1 and 100");
            }
        } catch (const std::exception&) {
            LOG_WARNING_FORMAT("GAME", "Invalid TPS value: %s", tps_str.c_str());
        }
    }

    void ReloadConfig() {
        LOG_INFO("GAME", "Reloading configuration...");

        if (std::filesystem::exists("config/game_server.conf")) {
            if (Common::GameServerConfig::LoadConfig("config/game_server.conf")) {
                // 새 설정 적용
                std::string new_log_level = Common::GameServerConfig::GetLogLevel();
                if (new_log_level != log_level_) {
                    log_level_ = new_log_level;
                    Common::LogManager::Instance().SetLogLevel(StringToLogLevel(log_level_));
                    LOG_INFO_FORMAT("GAME", "Log level changed to: %s", log_level_.c_str());
                }

                int new_tps = Common::GameServerConfig::GetTickRate();
                if (new_tps != game_tick_rate_) {
                    game_tick_rate_ = new_tps;
                    LOG_INFO_FORMAT("GAME", "TPS changed to: %d", game_tick_rate_);
                }

                LOG_INFO("GAME", "Configuration reloaded successfully");
            } else {
                LOG_ERROR("GAME", "Failed to reload configuration file");
            }
        } else {
            LOG_WARNING("GAME", "Configuration file not found, using current settings");
        }
    }

    void PrintHelp() {
        LOG_INFO("GAME", "=== Available Commands ===");
        LOG_INFO("GAME", "status      - Show server status");
        LOG_INFO("GAME", "players     - Show active players");
        LOG_INFO("GAME", "tps <rate>  - Change tick rate (1-100)");
        LOG_INFO("GAME", "reload      - Reload configuration");
        LOG_INFO("GAME", "help        - Show this help");
        LOG_INFO("GAME", "quit        - Shutdown server");
    }

    Network::NetworkManager network_manager_;
    int port_;
    int max_connections_;
    int game_tick_rate_;
    std::string log_level_;
    std::atomic<bool> game_running_;
    std::thread game_thread_;
    std::map<uint32_t, PlayerSession> player_sessions_;
    std::mutex players_mutex_;
};

int main() {
    try {
        // 설정 파일이 있으면 로드
        if (std::filesystem::exists("config/game_server.conf")) {
            if (!Common::GameServerConfig::LoadConfig("config/game_server.conf")) {
                std::cerr << "Warning: Failed to load config file, using defaults" << std::endl;
            }
        } else {
            // 기본 설정 파일 생성
            std::filesystem::create_directories("config");
            Common::GameServerConfig::SaveDefaultConfig("config/game_server.conf");
            std::cout << "Created default configuration file: config/game_server.conf" << std::endl;
        }

        // 로그 디렉토리 생성
        std::filesystem::create_directories("logs");

        GameServer server;

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