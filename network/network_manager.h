// network/network_manager.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <map>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #ifdef __linux__
        #include <sys/epoll.h>
    #elif __APPLE__
        #include <sys/event.h>
        #include <sys/time.h>
    #endif
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

namespace Network {

struct Packet {
    uint16_t type;
    uint16_t size;
    std::vector<uint8_t> data;

    Packet() : type(0), size(0) {}
    Packet(uint16_t t, const std::vector<uint8_t>& d) : type(t), size(d.size()), data(d) {}
};

class Connection {
public:
    Connection(SOCKET socket, const std::string& address);
    ~Connection();

    bool Send(const Packet& packet);
    bool Receive(Packet& packet);
    bool IsConnected() const { return connected_; }
    void Disconnect();

    SOCKET GetSocket() const { return socket_; }
    const std::string& GetAddress() const { return address_; }
    uint32_t GetId() const { return id_; }

private:
    SOCKET socket_;
    std::string address_;
    uint32_t id_;
    std::atomic<bool> connected_;
    mutable std::mutex send_mutex_;
    mutable std::mutex recv_mutex_;

    static std::atomic<uint32_t> next_id_;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // 서버 초기화
    bool InitializeServer(int port, int max_connections = 1000);

    // 클라이언트 초기화
    bool InitializeClient();

    // 서버 연결 (클라이언트용)
    std::shared_ptr<Connection> ConnectToServer(const std::string& host, int port);

    // 서버 시작/중지
    void StartServer();
    void StopServer();

    // 이벤트 콜백 설정
    void SetOnClientConnected(std::function<void(std::shared_ptr<Connection>)> callback);
    void SetOnClientDisconnected(std::function<void(std::shared_ptr<Connection>)> callback);
    void SetOnPacketReceived(std::function<void(std::shared_ptr<Connection>, const Packet&)> callback);

    // 패킷 전송
    bool SendToClient(std::shared_ptr<Connection> connection, const Packet& packet);
    bool SendToAll(const Packet& packet);

    // 연결 관리
    std::vector<std::shared_ptr<Connection>> GetConnections() const;
    void DisconnectClient(std::shared_ptr<Connection> connection);

    // 상태 확인
    bool IsServerRunning() const { return server_running_; }
    int GetConnectionCount() const;

private:
    void ServerThread();
    void ClientHandlerThread(std::shared_ptr<Connection> connection);
    void CleanupConnections();

    SOCKET server_socket_;
    std::atomic<bool> server_running_;
    std::atomic<bool> shutdown_requested_;

    std::thread server_thread_;
    std::vector<std::thread> client_threads_;

    std::vector<std::shared_ptr<Connection>> connections_;
    mutable std::mutex connections_mutex_;

    // 콜백 함수들
    std::function<void(std::shared_ptr<Connection>)> on_client_connected_;
    std::function<void(std::shared_ptr<Connection>)> on_client_disconnected_;
    std::function<void(std::shared_ptr<Connection>, const Packet&)> on_packet_received_;

    int max_connections_;
    int server_port_;

#ifdef _WIN32
    bool wsa_initialized_;
#elif __linux__
    int epoll_fd_;
#elif __APPLE__
    int kqueue_fd_;
#endif
};

// 패킷 타입 정의
enum PacketType : uint16_t {
    PACKET_ECHO = 1,
    PACKET_AUTH_REQUEST = 100,
    PACKET_AUTH_RESPONSE = 101,
    PACKET_LOGIN_REQUEST = 102,
    PACKET_LOGIN_RESPONSE = 103,
    PACKET_GAME_DATA = 200,
    PACKET_PLAYER_MOVE = 201,
    PACKET_PLAYER_CHAT = 202,
    PACKET_ZONE_CHANGE = 300,
    PACKET_ZONE_DATA = 301
};

// 유틸리티 함수들
std::vector<uint8_t> SerializeString(const std::string& str);
std::string DeserializeString(const std::vector<uint8_t>& data, size_t& offset);
void SerializeInt32(std::vector<uint8_t>& data, int32_t value);
int32_t DeserializeInt32(const std::vector<uint8_t>& data, size_t& offset);

} // namespace Network