// game_server/main.cpp
#include "../network/network_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <map>

class GameServer {
public:
    GameServer() : port_(8003), game_tick_rate_(20) {}

    bool Initialize() {
        if (!network_manager_.InitializeServer(port_)) {
            std::cerr << "Failed to initialize Game Server on port " << port_ << std::endl;
            return false;
        }

        // 콜백 설정
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[GAME] Player connected: " << conn->GetAddress() << std::endl;
            // 플레이어 세션 초기화
            player_sessions_[conn->GetId()] = {conn->GetId(), conn->GetAddress(), 0, 0};
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[GAME] Player disconnected: " << conn->GetAddress() << std::endl;
            // 플레이어 세션 제거
            player_sessions_.erase(conn->GetId());
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });

        return true;
    }

    void Run() {
        std::cout << "Starting Game Server on port " << port_ << " (TPS: " << game_tick_rate_ << ")" << std::endl;
        network_manager_.StartServer();

        // 게임 루프 스레드 시작
        game_running_ = true;
        game_thread_ = std::thread(&GameServer::GameLoop, this);

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "status") {
                std::cout << "Connected players: " << network_manager_.GetConnectionCount() << std::endl;
                std::cout << "Active sessions: " << player_sessions_.size() << std::endl;
            } else if (input == "players") {
                for (const auto& [id, session] : player_sessions_) {
                    std::cout << "Player ID: " << id << ", Address: " << session.address
                              << ", Pos: (" << session.x << ", " << session.y << ")" << std::endl;
                }
            }
        }

        // 게임 루프 종료
        game_running_ = false;
        if (game_thread_.joinable()) {
            game_thread_.join();
        }

        network_manager_.StopServer();
    }

private:
    struct PlayerSession {
        uint32_t player_id;
        std::string address;
        int32_t x, y;
    };

    void GameLoop() {
        const auto tick_duration = std::chrono::milliseconds(1000 / game_tick_rate_);
        auto last_tick = std::chrono::steady_clock::now();

        while (game_running_) {
            auto current_time = std::chrono::steady_clock::now();
            auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_tick);

            if (delta_time >= tick_duration) {
                // 게임 로직 업데이트 (현재는 빈 구현)
                UpdateGame();
                last_tick = current_time;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void UpdateGame() {
        // 20 TPS 게임 루프 - 현재는 빈 구현
        // 실제 게임에서는 플레이어 위치 동기화, 게임 로직 처리 등을 수행
    }

    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                // 에코 응답
                std::string message = "GAME_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[GAME] Echo request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_PLAYER_MOVE: {
                // 플레이어 이동 처리 (간단한 에코)
                if (player_sessions_.find(conn->GetId()) != player_sessions_.end()) {
                    // 실제로는 패킷에서 좌표를 파싱해야 함
                    player_sessions_[conn->GetId()].x += 1;
                    player_sessions_[conn->GetId()].y += 1;

                    std::string move_response = "MOVE_SUCCESS";
                    auto response_data = Network::SerializeString(move_response);
                    Network::Packet response(Network::PACKET_PLAYER_MOVE, response_data);
                    network_manager_.SendToClient(conn, response);
                    std::cout << "[GAME] Player move from " << conn->GetAddress() << std::endl;
                }
                break;
            }
            case Network::PACKET_PLAYER_CHAT: {
                // 채팅 메시지 브로드캐스트
                std::string chat_message = "CHAT_BROADCAST";
                auto response_data = Network::SerializeString(chat_message);
                Network::Packet response(Network::PACKET_PLAYER_CHAT, response_data);
                network_manager_.SendToAll(response);
                std::cout << "[GAME] Chat message from " << conn->GetAddress() << std::endl;
                break;
            }
            default:
                std::cout << "[GAME] Unknown packet type: " << packet.type << " from " << conn->GetAddress() << std::endl;
                break;
        }
    }

    Network::NetworkManager network_manager_;
    int port_;
    int game_tick_rate_;
    std::atomic<bool> game_running_;
    std::thread game_thread_;
    std::map<uint32_t, PlayerSession> player_sessions_;
};

int main() {
    GameServer server;

    if (!server.Initialize()) {
        return -1;
    }

    server.Run();
    return 0;
}