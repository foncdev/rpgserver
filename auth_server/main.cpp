#include "../network/network_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

class AuthServer {
public:
    AuthServer() : port_(8001) {}

    bool Initialize() {
        if (!network_manager_.InitializeServer(port_)) {
            std::cerr << "Failed to initialize Auth Server on port " << port_ << std::endl;
            return false;
        }

        // 콜백 설정
        network_manager_.SetOnClientConnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[AUTH] Client connected: " << conn->GetAddress() << std::endl;
        });

        network_manager_.SetOnClientDisconnected([this](std::shared_ptr<Network::Connection> conn) {
            std::cout << "[AUTH] Client disconnected: " << conn->GetAddress() << std::endl;
        });

        network_manager_.SetOnPacketReceived([this](std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
            HandlePacket(conn, packet);
        });

        return true;
    }

    void Run() {
        std::cout << "Starting Authentication Server on port " << port_ << std::endl;
        network_manager_.StartServer();

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "status") {
                std::cout << "Connected clients: " << network_manager_.GetConnectionCount() << std::endl;
            }
        }

        network_manager_.StopServer();
    }

private:
    void HandlePacket(std::shared_ptr<Network::Connection> conn, const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                // 에코 응답
                std::string message = "AUTH_ECHO_RESPONSE";
                auto response_data = Network::SerializeString(message);
                Network::Packet response(Network::PACKET_ECHO, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[AUTH] Echo request from " << conn->GetAddress() << std::endl;
                break;
            }
            case Network::PACKET_AUTH_REQUEST: {
                // 인증 요청 처리 (간단한 에코)
                std::string auth_response = "AUTH_SUCCESS";
                auto response_data = Network::SerializeString(auth_response);
                Network::Packet response(Network::PACKET_AUTH_RESPONSE, response_data);
                network_manager_.SendToClient(conn, response);
                std::cout << "[AUTH] Authentication request from " << conn->GetAddress() << std::endl;
                break;
            }
            default:
                std::cout << "[AUTH] Unknown packet type: " << packet.type << " from " << conn->GetAddress() << std::endl;
                break;
        }
    }

    Network::NetworkManager network_manager_;
    int port_;
};

int main() {
    AuthServer server;

    if (!server.Initialize()) {
        return -1;
    }

    server.Run();
    return 0;
}