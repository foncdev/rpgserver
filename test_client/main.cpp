#include "../network/network_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

class TestClient {
public:
    TestClient() {}

    bool Initialize() {
        if (!network_manager_.InitializeClient()) {
            std::cerr << "Failed to initialize test client" << std::endl;
            return false;
        }
        return true;
    }

    void Run() {
        std::cout << "MMORPG Test Client" << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "  connect <server> <port> - Connect to server" << std::endl;
        std::cout << "  disconnect - Disconnect from server" << std::endl;
        std::cout << "  echo <message> - Send echo message" << std::endl;
        std::cout << "  auth - Send authentication request" << std::endl;
        std::cout << "  login - Send login request" << std::endl;
        std::cout << "  move - Send move command" << std::endl;
        std::cout << "  chat <message> - Send chat message" << std::endl;
        std::cout << "  zone - Request zone data" << std::endl;
        std::cout << "  quit - Exit client" << std::endl;
        std::cout << std::endl;

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input.empty()) continue;

            std::vector<std::string> tokens = SplitString(input, ' ');
            std::string command = tokens[0];

            if (command == "quit" || command == "exit") {
                break;
            } else if (command == "connect" && tokens.size() >= 3) {
                std::string host = tokens[1];
                int port = std::stoi(tokens[2]);
                ConnectToServer(host, port);
            } else if (command == "disconnect") {
                DisconnectFromServer();
            } else if (command == "echo") {
                std::string message = (tokens.size() > 1) ? tokens[1] : "TEST_ECHO";
                SendEcho(message);
            } else if (command == "auth") {
                SendAuth();
            } else if (command == "login") {
                SendLogin();
            } else if (command == "move") {
                SendMove();
            } else if (command == "chat") {
                std::string message = (tokens.size() > 1) ? tokens[1] : "Hello World!";
                SendChat(message);
            } else if (command == "zone") {
                SendZoneRequest();
            } else if (command == "status") {
                std::cout << "Connection status: " << (connection_ ? "Connected" : "Disconnected") << std::endl;
            } else {
                std::cout << "Unknown command: " << command << std::endl;
            }
        }

        DisconnectFromServer();
    }

private:
    void ConnectToServer(const std::string& host, int port) {
        if (connection_) {
            std::cout << "Already connected. Disconnect first." << std::endl;
            return;
        }

        connection_ = network_manager_.ConnectToServer(host, port);
        if (connection_) {
            std::cout << "Connected to " << host << ":" << port << std::endl;

            // 수신 스레드 시작
            receiving_ = true;
            receive_thread_ = std::thread(&TestClient::ReceiveLoop, this);
        } else {
            std::cout << "Failed to connect to " << host << ":" << port << std::endl;
        }
    }

    void DisconnectFromServer() {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        receiving_ = false;
        connection_->Disconnect();

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        connection_.reset();
        std::cout << "Disconnected from server." << std::endl;
    }

    void ReceiveLoop() {
        while (receiving_ && connection_ && connection_->IsConnected()) {
            Network::Packet packet;
            if (connection_->Receive(packet)) {
                HandleReceivedPacket(packet);
            } else {
                break;
            }
        }
        receiving_ = false;
    }

    void HandleReceivedPacket(const Network::Packet& packet) {
        switch (packet.type) {
            case Network::PACKET_ECHO: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Echo: " << message << std::endl;
                break;
            }
            case Network::PACKET_AUTH_RESPONSE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Auth Response: " << message << std::endl;
                break;
            }
            case Network::PACKET_LOGIN_RESPONSE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Login Response: " << message << std::endl;
                break;
            }
            case Network::PACKET_PLAYER_MOVE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Move Response: " << message << std::endl;
                break;
            }
            case Network::PACKET_PLAYER_CHAT: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Chat: " << message << std::endl;
                break;
            }
            case Network::PACKET_ZONE_DATA: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                std::cout << "[RECEIVED] Zone Data: " << message << std::endl;
                break;
            }
            default:
                std::cout << "[RECEIVED] Unknown packet type: " << packet.type << std::endl;
                break;
        }
    }

    void SendEcho(const std::string& message) {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        auto data = Network::SerializeString(message);
        Network::Packet packet(Network::PACKET_ECHO, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Echo: " << message << std::endl;
        } else {
            std::cout << "Failed to send echo message." << std::endl;
        }
    }

    void SendAuth() {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        std::string auth_data = "test_user:test_password";
        auto data = Network::SerializeString(auth_data);
        Network::Packet packet(Network::PACKET_AUTH_REQUEST, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Authentication request" << std::endl;
        } else {
            std::cout << "Failed to send authentication request." << std::endl;
        }
    }

    void SendLogin() {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        std::string login_data = "test_user";
        auto data = Network::SerializeString(login_data);
        Network::Packet packet(Network::PACKET_LOGIN_REQUEST, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Login request" << std::endl;
        } else {
            std::cout << "Failed to send login request." << std::endl;
        }
    }

    void SendMove() {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        std::string move_data = "move_right";
        auto data = Network::SerializeString(move_data);
        Network::Packet packet(Network::PACKET_PLAYER_MOVE, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Move command" << std::endl;
        } else {
            std::cout << "Failed to send move command." << std::endl;
        }
    }

    void SendChat(const std::string& message) {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        auto data = Network::SerializeString(message);
        Network::Packet packet(Network::PACKET_PLAYER_CHAT, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Chat: " << message << std::endl;
        } else {
            std::cout << "Failed to send chat message." << std::endl;
        }
    }

    void SendZoneRequest() {
        if (!connection_) {
            std::cout << "Not connected." << std::endl;
            return;
        }

        std::string zone_request = "request_zone_data";
        auto data = Network::SerializeString(zone_request);
        Network::Packet packet(Network::PACKET_ZONE_DATA, data);
        if (connection_->Send(packet)) {
            std::cout << "[SENT] Zone data request" << std::endl;
        } else {
            std::cout << "Failed to send zone data request." << std::endl;
        }
    }

    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;

        for (char c : str) {
            if (c == delimiter) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }

        if (!token.empty()) {
            tokens.push_back(token);
        }

        return tokens;
    }

    Network::NetworkManager network_manager_;
    std::shared_ptr<Network::Connection> connection_;
    std::thread receive_thread_;
    std::atomic<bool> receiving_;
};

int main() {
    TestClient client;

    if (!client.Initialize()) {
        return -1;
    }

    client.Run();
    return 0;
}