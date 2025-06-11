// test_client/main.cpp - Updated with Logging
#include "../network/network_manager.h"
#include "../common/log_manager.h"
#include "../common/config_manager.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

class TestClient {
public:
    TestClient() : receiving_(false) {
        // 클라이언트용 로그 설정
        Common::LogManager::Instance().SetLogLevel(Common::LogLevel::INFO);
        Common::LogManager::Instance().SetConsoleOutput(true);
        Common::LogManager::Instance().SetFileOutput(true, "logs/test_client.log");
    }

    bool Initialize() {
        LOG_INFO("CLIENT", "Initializing MMORPG Test Client...");

        if (!network_manager_.InitializeClient()) {
            LOG_ERROR("CLIENT", "Failed to initialize test client");
            return false;
        }

        LOG_INFO("CLIENT", "Test client initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO("CLIENT", "=== MMORPG Test Client ===");
        PrintCommands();

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input.empty()) continue;

            std::vector<std::string> tokens = SplitString(input, ' ');
            std::string command = tokens[0];

            try {
                if (command == "quit" || command == "exit") {
                    LOG_INFO("CLIENT", "Exit requested by user");
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
                    std::string message = (tokens.size() > 1) ?
                        input.substr(input.find(' ') + 1) : "Hello World!";
                    SendChat(message);
                } else if (command == "zone") {
                    SendZoneRequest();
                } else if (command == "status") {
                    PrintStatus();
                } else if (command == "help" || command == "commands") {
                    PrintCommands();
                } else if (command == "spam" && tokens.size() >= 2) {
                    int count = std::stoi(tokens[1]);
                    SpamTest(count);
                } else if (command == "stress" && tokens.size() >= 2) {
                    int duration = std::stoi(tokens[1]);
                    StressTest(duration);
                } else {
                    LOG_WARNING_FORMAT("CLIENT", "Unknown command: %s", command.c_str());
                    std::cout << "Type 'help' for available commands." << std::endl;
                }
            } catch (const std::exception& e) {
                LOG_ERROR_FORMAT("CLIENT", "Error processing command '%s': %s",
                               command.c_str(), e.what());
            }
        }

        DisconnectFromServer();
        LOG_INFO("CLIENT", "Test client shutting down");
    }

private:
    void ConnectToServer(const std::string& host, int port) {
        if (connection_) {
            LOG_WARNING("CLIENT", "Already connected. Disconnect first.");
            return;
        }

        LOG_INFO_FORMAT("CLIENT", "Connecting to %s:%d...", host.c_str(), port);
        connection_ = network_manager_.ConnectToServer(host, port);

        if (connection_) {
            LOG_INFO_FORMAT("CLIENT", "Successfully connected to %s:%d", host.c_str(), port);

            // 수신 스레드 시작
            receiving_ = true;
            receive_thread_ = std::thread(&TestClient::ReceiveLoop, this);
        } else {
            LOG_ERROR_FORMAT("CLIENT", "Failed to connect to %s:%d", host.c_str(), port);
        }
    }

    void DisconnectFromServer() {
        if (!connection_) {
            LOG_INFO("CLIENT", "Not connected to any server");
            return;
        }

        LOG_INFO("CLIENT", "Disconnecting from server...");
        receiving_ = false;
        connection_->Disconnect();

        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }

        connection_.reset();
        LOG_INFO("CLIENT", "Disconnected from server");
    }

    void ReceiveLoop() {
        LOG_DEBUG("CLIENT", "Receive loop started");

        while (receiving_ && connection_ && connection_->IsConnected()) {
            Network::Packet packet;
            if (connection_->Receive(packet)) {
                HandleReceivedPacket(packet);
            } else {
                if (receiving_) {
                    LOG_WARNING("CLIENT", "Connection lost during receive");
                }
                break;
            }
        }

        receiving_ = false;
        LOG_DEBUG("CLIENT", "Receive loop ended");
    }

    void HandleReceivedPacket(const Network::Packet& packet) {
        LOG_DEBUG_FORMAT("CLIENT", "Received packet type: %d, size: %d",
                        packet.type, packet.size);

        switch (packet.type) {
            case Network::PACKET_ECHO: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[ECHO] %s", message.c_str());
                break;
            }
            case Network::PACKET_AUTH_RESPONSE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[AUTH] %s", message.c_str());
                break;
            }
            case Network::PACKET_LOGIN_RESPONSE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[LOGIN] %s", message.c_str());
                break;
            }
            case Network::PACKET_PLAYER_MOVE: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[MOVE] %s", message.c_str());
                break;
            }
            case Network::PACKET_PLAYER_CHAT: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[CHAT] %s", message.c_str());
                break;
            }
            case Network::PACKET_ZONE_DATA: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[ZONE] %s", message.c_str());
                break;
            }
            case Network::PACKET_GAME_DATA: {
                size_t offset = 0;
                std::string message = Network::DeserializeString(packet.data, offset);
                LOG_INFO_FORMAT("CLIENT", "[GAME] %s", message.c_str());
                break;
            }
            default:
                LOG_WARNING_FORMAT("CLIENT", "Unknown packet type received: %d", packet.type);
                break;
        }
    }

    void SendEcho(const std::string& message) {
        if (!CheckConnection()) return;

        auto data = Network::SerializeString(message);
        Network::Packet packet(Network::PACKET_ECHO, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG_FORMAT("CLIENT", "Sent echo: %s", message.c_str());
        } else {
            LOG_ERROR("CLIENT", "Failed to send echo message");
        }
    }

    void SendAuth() {
        if (!CheckConnection()) return;

        std::string auth_data = "test_user:test_password";
        auto data = Network::SerializeString(auth_data);
        Network::Packet packet(Network::PACKET_AUTH_REQUEST, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG("CLIENT", "Sent authentication request");
        } else {
            LOG_ERROR("CLIENT", "Failed to send authentication request");
        }
    }

    void SendLogin() {
        if (!CheckConnection()) return;

        std::string login_data = "test_user";
        auto data = Network::SerializeString(login_data);
        Network::Packet packet(Network::PACKET_LOGIN_REQUEST, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG("CLIENT", "Sent login request");
        } else {
            LOG_ERROR("CLIENT", "Failed to send login request");
        }
    }

    void SendMove() {
        if (!CheckConnection()) return;

        std::string move_data = "move_right";
        auto data = Network::SerializeString(move_data);
        Network::Packet packet(Network::PACKET_PLAYER_MOVE, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG("CLIENT", "Sent move command");
        } else {
            LOG_ERROR("CLIENT", "Failed to send move command");
        }
    }

    void SendChat(const std::string& message) {
        if (!CheckConnection()) return;

        auto data = Network::SerializeString(message);
        Network::Packet packet(Network::PACKET_PLAYER_CHAT, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG_FORMAT("CLIENT", "Sent chat: %s", message.c_str());
        } else {
            LOG_ERROR("CLIENT", "Failed to send chat message");
        }
    }

    void SendZoneRequest() {
        if (!CheckConnection()) return;

        std::string zone_request = "request_zone_data";
        auto data = Network::SerializeString(zone_request);
        Network::Packet packet(Network::PACKET_ZONE_DATA, data);
        if (connection_->Send(packet)) {
            LOG_DEBUG("CLIENT", "Sent zone data request");
        } else {
            LOG_ERROR("CLIENT", "Failed to send zone data request");
        }
    }

    void SpamTest(int count) {
        if (!CheckConnection()) return;

        LOG_INFO_FORMAT("CLIENT", "Starting spam test with %d messages...", count);
        auto start_time = std::chrono::steady_clock::now();

        for (int i = 0; i < count; ++i) {
            std::string message = "spam_message_" + std::to_string(i);
            SendEcho(message);

            // 너무 빠르게 보내지 않도록 약간의 지연
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        LOG_INFO_FORMAT("CLIENT", "Spam test completed: %d messages in %lld ms",
                       count, duration.count());
    }

    void StressTest(int duration_seconds) {
        if (!CheckConnection()) return;

        LOG_INFO_FORMAT("CLIENT", "Starting stress test for %d seconds...", duration_seconds);
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);

        int message_count = 0;
        while (std::chrono::steady_clock::now() < end_time && connection_->IsConnected()) {
            std::string message = "stress_test_" + std::to_string(message_count++);
            SendEcho(message);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        LOG_INFO_FORMAT("CLIENT", "Stress test completed: %d messages sent", message_count);
    }

    void PrintStatus() {
        if (connection_) {
            LOG_INFO_FORMAT("CLIENT", "Connection Status: Connected to %s",
                           connection_->GetAddress().c_str());
            LOG_INFO_FORMAT("CLIENT", "Connection ID: %d", connection_->GetId());
            LOG_INFO_FORMAT("CLIENT", "Connection Active: %s",
                           connection_->IsConnected() ? "Yes" : "No");
        } else {
            LOG_INFO("CLIENT", "Connection Status: Disconnected");
        }
        LOG_INFO_FORMAT("CLIENT", "Receive Thread Active: %s", receiving_ ? "Yes" : "No");
    }

    void PrintCommands() {
        std::cout << "\n=== Available Commands ===" << std::endl;
        std::cout << "connect <host> <port>  - Connect to server" << std::endl;
        std::cout << "disconnect             - Disconnect from server" << std::endl;
        std::cout << "echo <message>         - Send echo message" << std::endl;
        std::cout << "auth                   - Send authentication request" << std::endl;
        std::cout << "login                  - Send login request" << std::endl;
        std::cout << "move                   - Send move command" << std::endl;
        std::cout << "chat <message>         - Send chat message" << std::endl;
        std::cout << "zone                   - Request zone data" << std::endl;
        std::cout << "spam <count>           - Send multiple echo messages" << std::endl;
        std::cout << "stress <seconds>       - Stress test for specified duration" << std::endl;
        std::cout << "status                 - Show connection status" << std::endl;
        std::cout << "help                   - Show this help" << std::endl;
        std::cout << "quit                   - Exit client" << std::endl;
        std::cout << "\nExample usage:" << std::endl;
        std::cout << "  connect localhost 8001  # Connect to auth server" << std::endl;
        std::cout << "  connect localhost 8003  # Connect to game server" << std::endl;
        std::cout << "  chat Hello everyone!    # Send chat message" << std::endl;
        std::cout << "  spam 100               # Send 100 echo messages" << std::endl;
        std::cout << std::endl;
    }

    bool CheckConnection() {
        if (!connection_) {
            LOG_WARNING("CLIENT", "Not connected to any server. Use 'connect <host> <port>' first.");
            return false;
        }
        if (!connection_->IsConnected()) {
            LOG_WARNING("CLIENT", "Connection is not active");
            return false;
        }
        return true;
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
    try {
        // 로그 디렉토리 생성
        std::filesystem::create_directories("logs");

        TestClient client;

        if (!client.Initialize()) {
            return -1;
        }

        client.Run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}