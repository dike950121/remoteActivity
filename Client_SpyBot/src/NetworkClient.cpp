#include "NetworkClient.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
#endif

/**
 * @brief Constructor for NetworkClient
 * @param server_ip Server IP address to connect to
 * @param server_port Server port number
 */
NetworkClient::NetworkClient(const std::string& server_ip, int server_port) 
    : server_ip_(server_ip), server_port_(server_port), client_socket_(INVALID_SOCKET),
      connected_(false), listening_(false) {
}

/**
 * @brief Destructor - ensures proper cleanup of socket resources
 */
NetworkClient::~NetworkClient() {
    Disconnect();
    CleanupWinsock();
}

/**
 * @brief Initialize the network client and establish connection
 * @return true if initialization successful, false otherwise
 */
bool NetworkClient::Initialize() {
#ifdef _WIN32
    return InitializeWinsock();
#else
    return true;
#endif
}

/**
 * @brief Connect to the remote server
 * @return true if connection successful, false otherwise
 */
bool NetworkClient::Connect() {
    if (connected_) {
        return true;
    }
    
    // Create socket
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_ == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return false;
    }
    
    // Configure server address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);
    
#ifdef _WIN32
    server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "[ERROR] Invalid server IP address" << std::endl;
        return false;
    }
#else
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[ERROR] Invalid server IP address" << std::endl;
        return false;
    }
#endif
    
    // Attempt connection
    if (connect(client_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Failed to connect to server" << std::endl;
#ifdef _WIN32
        closesocket(client_socket_);
#else
        close(client_socket_);
#endif
        client_socket_ = INVALID_SOCKET;
        return false;
    }
    
    connected_ = true;
    std::cout << "[INFO] Connected to server " << server_ip_ << ":" << server_port_ << std::endl;
    return true;
}

/**
 * @brief Send JSON data to the server
 * @param json_data JSON formatted string to send
 * @return true if data sent successfully, false otherwise
 */
bool NetworkClient::SendData(const std::string& json_data) {
    if (!connected_ || client_socket_ == INVALID_SOCKET) {
        std::cerr << "[ERROR] Not connected to server" << std::endl;
        return false;
    }
    
    // Prepare data with length prefix
    uint32_t data_length = htonl(static_cast<uint32_t>(json_data.length()));
    
    // Send data length first
    if (send(client_socket_, reinterpret_cast<const char*>(&data_length), sizeof(data_length), 0) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Failed to send data length" << std::endl;
        return false;
    }
    
    // Send actual data
    const char* data_ptr = json_data.c_str();
    size_t total_sent = 0;
    size_t data_size = json_data.length();
    
    while (total_sent < data_size) {
        int sent = send(client_socket_, data_ptr + total_sent, static_cast<int>(data_size - total_sent), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "[ERROR] Failed to send data" << std::endl;
            return false;
        }
        total_sent += sent;
    }
    
    return true;
}

/**
 * @brief Disconnect from the server and cleanup
 */
void NetworkClient::Disconnect() {
    if (!connected_) {
        return;
    }
    
    connected_ = false;
    
    if (client_socket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(client_socket_);
#else
        close(client_socket_);
#endif
        client_socket_ = INVALID_SOCKET;
    }
    
    std::cout << "[INFO] Disconnected from server" << std::endl;
}

/**
 * @brief Check if client is currently connected
 * @return true if connected, false otherwise
 */
bool NetworkClient::IsConnected() const {
    return connected_;
}

/**
 * @brief Set callback function for received data
 * @param callback Function to call when data is received
 */
void NetworkClient::SetDataReceivedCallback(std::function<void(const std::string&)> callback) {
    data_callback_ = callback;
}

/**
 * @brief Start listening for incoming data from server
 */
void NetworkClient::StartListening() {
    if (listening_ || !connected_) {
        return;
    }
    
    listening_ = true;
    listen_thread_ = std::thread(&NetworkClient::ListenForData, this);
}

/**
 * @brief Stop listening for incoming data
 */
void NetworkClient::StopListening() {
    if (!listening_) {
        return;
    }
    
    listening_ = false;
    
    if (listen_thread_.joinable()) {
        listen_thread_.join();
    }
}

/**
 * @brief Background thread function for listening to server responses
 */
void NetworkClient::ListenForData() {
    char buffer[4096];
    
    while (listening_ && connected_) {
        // First, receive the data length
        uint32_t data_length;
        int received = recv(client_socket_, reinterpret_cast<char*>(&data_length), sizeof(data_length), 0);
        
        if (received <= 0) {
            if (listening_) {
                std::cerr << "[ERROR] Connection lost while receiving data length" << std::endl;
                connected_ = false;
            }
            break;
        }
        
        data_length = ntohl(data_length);
        if (data_length == 0 || data_length > 1048576) { // Max 1MB
            std::cerr << "[ERROR] Invalid data length received: " << data_length << std::endl;
            continue;
        }
        
        // Receive the actual data
        std::string received_data;
        size_t total_received = 0;
        
        while (total_received < data_length && listening_ && connected_) {
            size_t to_receive = std::min(static_cast<size_t>(sizeof(buffer)), data_length - total_received);
            received = recv(client_socket_, buffer, static_cast<int>(to_receive), 0);
            
            if (received <= 0) {
                if (listening_) {
                    std::cerr << "[ERROR] Connection lost while receiving data" << std::endl;
                    connected_ = false;
                }
                break;
            }
            
            received_data.append(buffer, received);
            total_received += received;
        }
        
        // Process received data
        if (total_received == data_length && data_callback_) {
            data_callback_(received_data);
        }
    }
}

/**
 * @brief Initialize Winsock on Windows platform
 * @return true if successful, false otherwise
 */
bool NetworkClient::InitializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[ERROR] WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;
#else
    return true;
#endif
}

/**
 * @brief Cleanup Winsock on Windows platform
 */
void NetworkClient::CleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
} 