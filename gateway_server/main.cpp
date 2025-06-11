// gateway_server/main.cpp
#include "../network/network_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

class GatewayServer {
public:
    GatewayServer() : port_(8002) {}

    bool Initialize() {
        if (!network_manager_.InitializeServer(port_)) {
            std::cerr << "Failed to initialize Gateway Server on port " << port_ << std::endl;
            return false;
        }

        // 콜백 설정
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[GATEWAY] Client connected: " << conn->GetAddress() << std::endl;
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[GATEWAY] Client disconnected: " << conn->GetAddress() << std::endl;
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });

        return true;
    }

    void Run() {
        std::cout << "Starting Gateway Server on port " << port_ << std::endl;
        network_manager_.StartServer();

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "status") {
                std::cout << "Connected clients: " << network_manager_.GetConnectionCount() << std::endl;
            } else if (input == "broadcast") {
                // 테스트용 브로드캐스트
                std::string message = "GATEWAY_BROADCAST_MESSAGE";
                auto data = Network::SerializeString(message);
                Network::Packet packet(Network::PACKET_ECHO, data);
                network_manager_.SendToAll(packet);
                std::cout << "[GATEWAY] Broadcast sent to all clients" << std::endl;
            }
        }

        network_manager_.StopServer();
    }

private:
    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                // 에코 응답
                std::string message = "GATEWAY_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[GATEWAY] Echo request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_LOGIN_REQUEST: {
                // 로그인 요청 처리 (간단한 에코)
                std::string login_response = "LOGIN_SUCCESS";
                auto response_data = Network::SerializeString(login_response);
                Network::Packet response(Network::PACKET_LOGIN_RESPONSE, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[GATEWAY] Login request from " << conn->GetAddress() << std::endl;
                break;
            }
            default:
                std::cout << "[GATEWAY] Unknown packet type: " << packet.type << " from " << conn->GetAddress() << std::endl;
                break;
        }
    }

    Network::NetworkManager network_manager_;
    int port_;
};

int main() {
    GatewayServer server;

    if (!server.Initialize()) {
        return -1;
    }

    server.Run();
    return 0;
}