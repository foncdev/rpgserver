#include "network_manager.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace Network {

std::atomic<uint32_t> Connection::next_id_(1);

Connection::Connection(SOCKET socket, const std::string& address)
    : socket_(socket), address_(address), connected_(true) {
    id_ = next_id_.fetch_add(1);
}

Connection::~Connection() {
    Disconnect();
}

bool Connection::Send(const Packet& packet) {
    if (!connected_) return false;

    std::lock_guard<std::mutex> lock(send_mutex_);

    // 패킷 헤더 전송 (타입 + 크기)
    uint16_t header[2] = { packet.type, packet.size };
    int sent = send(socket_, reinterpret_cast<const char*>(header), sizeof(header), 0);
    if (sent != sizeof(header)) {
        connected_ = false;
        return false;
    }

    // 패킷 데이터 전송
    if (packet.size > 0) {
        sent = send(socket_, reinterpret_cast<const char*>(packet.data.data()), packet.size, 0);
        if (sent != packet.size) {
            connected_ = false;
            return false;
        }
    }

    return true;
}

bool Connection::Receive(Packet& packet) {
    if (!connected_) return false;

    std::lock_guard<std::mutex> lock(recv_mutex_);

    // 패킷 헤더 수신
    uint16_t header[2];
    int received = recv(socket_, reinterpret_cast<char*>(header), sizeof(header), 0);
    if (received != sizeof(header)) {
        connected_ = false;
        return false;
    }

    packet.type = header[0];
    packet.size = header[1];

    // 패킷 데이터 수신
    if (packet.size > 0) {
        packet.data.resize(packet.size);
        received = recv(socket_, reinterpret_cast<char*>(packet.data.data()), packet.size, 0);
        if (received != packet.size) {
            connected_ = false;
            return false;
        }
    }

    return true;
}

void Connection::Disconnect() {
    if (connected_.exchange(false)) {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
    }
}

NetworkManager::NetworkManager()
    : server_socket_(INVALID_SOCKET)
    , server_running_(false)
    , shutdown_requested_(false)
    , max_connections_(1000)
    , server_port_(0) {
#ifdef _WIN32
    WSADATA wsaData;
    wsa_initialized_ = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
#elif __linux__
    epoll_fd_ = epoll_create1(0);
#elif __APPLE__
    kqueue_fd_ = kqueue();
#endif
}

NetworkManager::~NetworkManager() {
    StopServer();

#ifdef _WIN32
    if (wsa_initialized_) {
        WSACleanup();
    }
#elif __linux__
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
#elif __APPLE__
    if (kqueue_fd_ != -1) {
        close(kqueue_fd_);
    }
#endif
}

bool NetworkManager::InitializeServer(int port, int max_connections) {
    server_port_ = port;
    max_connections_ = max_connections;

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket" << std::endl;
        return false;
    }

    // 소켓 옵션 설정
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    // 바인드
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind server socket" << std::endl;
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return false;
    }

    // 리슨
    if (listen(server_socket_, max_connections) == SOCKET_ERROR) {
        std::cerr << "Failed to listen on server socket" << std::endl;
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return false;
    }

    std::cout << "Server initialized on port " << port << std::endl;
    return true;
}

bool NetworkManager::InitializeClient() {
    // 클라이언트는 초기화 작업이 특별히 필요 없음
    return true;
}

std::shared_ptr<Connection> NetworkManager::ConnectToServer(const std::string& host, int port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create client socket" << std::endl;
        return nullptr;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

#ifdef _WIN32
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        // hostname 해결 시도
        struct hostent* he = gethostbyname(host.c_str());
        if (he == nullptr) {
            std::cerr << "Failed to resolve hostname: " << host << std::endl;
            closesocket(client_socket);
            return nullptr;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
#else
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        closesocket(client_socket);
        return nullptr;
    }
#endif

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr),
                sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server" << std::endl;
        closesocket(client_socket);
        return nullptr;
    }

    return std::make_shared<Connection>(client_socket, host + ":" + std::to_string(port));
}

void NetworkManager::StartServer() {
    if (server_running_ || server_socket_ == INVALID_SOCKET) {
        return;
    }

    server_running_ = true;
    shutdown_requested_ = false;

    server_thread_ = std::thread(&NetworkManager::ServerThread, this);
    std::cout << "Server started on port " << server_port_ << std::endl;
}

void NetworkManager::StopServer() {
    if (!server_running_) return;

    shutdown_requested_ = true;
    server_running_ = false;

    // 서버 소켓 닫기
    if (server_socket_ != INVALID_SOCKET) {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }

    // 모든 클라이언트 연결 해제
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& conn : connections_) {
            conn->Disconnect();
        }
        connections_.clear();
    }

    // 스레드 종료 대기
    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    client_threads_.clear();

    std::cout << "Server stopped" << std::endl;
}

void NetworkManager::ServerThread() {
    while (server_running_ && !shutdown_requested_) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(server_socket_,
                                    reinterpret_cast<sockaddr*>(&client_addr),
                                    &client_addr_len);

        if (client_socket == INVALID_SOCKET) {
            if (server_running_) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        // 클라이언트 주소 문자열 생성
        char client_ip[INET_ADDRSTRLEN];
#ifdef _WIN32
        strcpy_s(client_ip, sizeof(client_ip), inet_ntoa(client_addr.sin_addr));
#else
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
#endif
        std::string client_address = std::string(client_ip) + ":" +
                                   std::to_string(ntohs(client_addr.sin_port));

        // 새 연결 생성
        auto connection = std::make_shared<Connection>(client_socket, client_address);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.push_back(connection);
        }

        // 클라이언트 핸들러 스레드 시작
        client_threads_.emplace_back(&NetworkManager::ClientHandlerThread, this, connection);

        // 연결 콜백 호출
        if (on_client_connected_) {
            on_client_connected_(connection);
        }

        std::cout << "Client connected: " << client_address
                  << " (ID: " << connection->GetId() << ")" << std::endl;
    }
}

void NetworkManager::ClientHandlerThread(std::shared_ptr<Connection> connection) {
    while (connection->IsConnected() && !shutdown_requested_) {
        Packet packet;
        if (connection->Receive(packet)) {
            // 패킷 수신 콜백 호출
            if (on_packet_received_) {
                on_packet_received_(connection, packet);
            }
        } else {
            break;
        }
    }

    // 연결 해제 처리
    connection->Disconnect();

    // 연결 목록에서 제거
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(
            std::remove(connections_.begin(), connections_.end(), connection),
            connections_.end()
        );
    }

    // 연결 해제 콜백 호출
    if (on_client_disconnected_) {
        on_client_disconnected_(connection);
    }

    std::cout << "Client disconnected: " << connection->GetAddress()
              << " (ID: " << connection->GetId() << ")" << std::endl;
}

void NetworkManager::SetOnClientConnected(std::function<void(std::shared_ptr<Connection>)> callback) {
    on_client_connected_ = callback;
}

void NetworkManager::SetOnClientDisconnected(std::function<void(std::shared_ptr<Connection>)> callback) {
    on_client_disconnected_ = callback;
}

void NetworkManager::SetOnPacketReceived(std::function<void(std::shared_ptr<Connection>, const Packet&)> callback) {
    on_packet_received_ = callback;
}

bool NetworkManager::SendToClient(std::shared_ptr<Connection> connection, const Packet& packet) {
    if (!connection || !connection->IsConnected()) {
        return false;
    }
    return connection->Send(packet);
}

bool NetworkManager::SendToAll(const Packet& packet) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    bool success = true;

    for (auto& connection : connections_) {
        if (connection->IsConnected()) {
            if (!connection->Send(packet)) {
                success = false;
            }
        }
    }

    return success;
}

std::vector<std::shared_ptr<Connection>> NetworkManager::GetConnections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_;
}

int NetworkManager::GetConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return static_cast<int>(connections_.size());
}

// 유틸리티 함수 구현
std::vector<uint8_t> SerializeString(const std::string& str) {
    std::vector<uint8_t> data;
    uint16_t length = static_cast<uint16_t>(str.length());

    data.push_back(length & 0xFF);
    data.push_back((length >> 8) & 0xFF);

    for (char c : str) {
        data.push_back(static_cast<uint8_t>(c));
    }

    return data;
}

std::string DeserializeString(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 2 > data.size()) return "";

    uint16_t length = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    if (offset + length > data.size()) return "";

    std::string str;
    str.reserve(length);

    for (uint16_t i = 0; i < length; ++i) {
        str.push_back(static_cast<char>(data[offset + i]));
    }

    offset += length;
    return str;
}

void SerializeInt32(std::vector<uint8_t>& data, int32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

int32_t DeserializeInt32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0;

    int32_t value = data[offset] |
                   (data[offset + 1] << 8) |
                   (data[offset + 2] << 16) |
                   (data[offset + 3] << 24);
    offset += 4;
    return value;
}

} // namespace Network