// zone_server/main.cpp
#include "../network/network_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <vector>

class ZoneServer {
public:
    ZoneServer() : port_(8004), zone_id_(1) {}

    bool Initialize() {
        if (!network_manager_.InitializeServer(port_)) {
            std::cerr << "Failed to initialize Zone Server on port " << port_ << std::endl;
            return false;
        }

        // 콜백 설정
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[ZONE-" << zone_id_ << "] Player entered zone: " << conn->GetAddress() << std::endl;
            // 존 플레이어 추가
            zone_players_[conn->GetId()] = {conn->GetId(), conn->GetAddress(), 100, 100};
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[ZONE-" << zone_id_ << "] Player left zone: " << conn->GetAddress() << std::endl;
            // 존 플레이어 제거
            zone_players_.erase(conn->GetId());
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });

        // 존 맵 초기화 (간단한 예시)
        InitializeZoneMap();

        return true;
    }

    void Run() {
        std::cout << "Starting Zone Server [Zone " << zone_id_ << "] on port " << port_ << std::endl;
        network_manager_.StartServer();

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "status") {
                std::cout << "Zone ID: " << zone_id_ << std::endl;
                std::cout << "Players in zone: " << network_manager_.GetConnectionCount() << std::endl;
                std::cout << "Map size: " << map_width_ << "x" << map_height_ << std::endl;
            } else if (input == "players") {
                for (const auto& [id, player] : zone_players_) {
                    std::cout << "Player ID: " << id << ", Address: " << player.address
                              << ", Zone Pos: (" << player.zone_x << ", " << player.zone_y << ")" << std::endl;
                }
            } else if (input == "map") {
                std::cout << "Zone Map Layout:" << std::endl;
                PrintZoneMap();
            }
        }

        network_manager_.StopServer();
    }

private:
    struct ZonePlayer {
        uint32_t player_id;
        std::string address;
        int32_t zone_x, zone_y;
    };

    void InitializeZoneMap() {
        map_width_ = 50;
        map_height_ = 50;

        // 간단한 맵 생성 (실제로는 파일에서 로드)
        zone_map_.resize(map_height_);
        for (int y = 0; y < map_height_; ++y) {
            zone_map_[y].resize(map_width_);
            for (int x = 0; x < map_width_; ++x) {
                // 경계는 벽, 나머지는 빈 공간
                if (x == 0 || x == map_width_ - 1 || y == 0 || y == map_height_ - 1) {
                    zone_map_[y][x] = '#'; // 벽
                } else {
                    zone_map_[y][x] = '.'; // 빈 공간
                }
            }
        }

        std::cout << "[ZONE-" << zone_id_ << "] Map initialized: " << map_width_ << "x" << map_height_ << std::endl;
    }

    void PrintZoneMap() {
        // 맵의 일부만 출력 (10x10)
        for (int y = 0; y < std::min(10, map_height_); ++y) {
            for (int x = 0; x < std::min(10, map_width_); ++x) {
                std::cout << zone_map_[y][x];
            }
            std::cout << std::endl;
        }
        if (map_width_ > 10 || map_height_ > 10) {
            std::cout << "... (showing 10x10 section of " << map_width_ << "x" << map_height_ << " map)" << std::endl;
        }
    }

    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                // 에코 응답
                std::string message = "ZONE_ECHO_RESPONSE_ZONE_" + std::to_string(zone_id_);
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[ZONE-" << zone_id_ << "] Echo request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_ZONE_CHANGE: {
                // 존 변경 요청 처리
                std::string zone_response = "ZONE_CHANGE_SUCCESS";
                auto response_data = Network::SerializeString(zone_response);
                Network::Packet response(Network::PACKET_ZONE_CHANGE, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[ZONE-" << zone_id_ << "] Zone change request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_ZONE_DATA: {
                // 존 데이터 요청 처리
                std::string zone_data = "ZONE_DATA_ZONE_" + std::to_string(zone_id_) + "_SIZE_" +
                                       std::to_string(map_width_) + "x" + std::to_string(map_height_);
                auto response_data = Network::SerializeString(zone_data);
                Network::Packet response(Network::PACKET_ZONE_DATA, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[ZONE-" << zone_id_ << "] Zone data request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_PLAYER_MOVE: {
                // 존 내 플레이어 이동 처리
                if (zone_players_.find(conn->GetId()) != zone_players_.end()) {
                    // 실제로는 패킷에서 좌표를 파싱하고 충돌 검사를 해야 함
                    auto& player = zone_players_[conn->GetId()];
                    player.zone_x = std::max(1, std::min(map_width_ - 2, player.zone_x + 1));
                    player.zone_y = std::max(1, std::min(map_height_ - 2, player.zone_y + 1));

                    std::string move_response = "ZONE_MOVE_SUCCESS";
                    auto response_data = Network::SerializeString(move_response);
                    Network::Packet response(Network::PACKET_PLAYER_MOVE, response_data);
                    network_manager_.SendToClient(conn, response);

                    // 주변 플레이어들에게 위치 동기화 (간단한 브로드캐스트)
                    std::string sync_message = "PLAYER_POSITION_SYNC";
                    auto sync_data = Network::SerializeString(sync_message);
                    Network::Packet sync_packet(Network::PACKET_GAME_DATA, sync_data);
                    network_manager_.SendToAll(sync_packet);

                    std::cout << "[ZONE-" << zone_id_ << "] Player move in zone from " << conn->GetAddress() << std::endl;
                }
                break;
            }
            default:
                std::cout << "[ZONE-" << zone_id_ << "] Unknown packet type: " << packet.type << " from " << conn->GetAddress() << std::endl;
                break;
        }
    }

    Network::NetworkManager network_manager_;
    int port_;
    int zone_id_;
    int map_width_, map_height_;
    std::vector<std::vector<char>> zone_map_;
    std::map<uint32_t, ZonePlayer> zone_players_;
};

int main() {
    ZoneServer server;

    if (!server.Initialize()) {
        return -1;
    }

    server.Run();
    return 0;
}