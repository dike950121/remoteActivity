#include "network/NetworkManager.h"
#include "common/Logger.h"
#include "common/Config.h"
#include "common/Protocol.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>

#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace Network {
    
    NetworkManager::NetworkManager()
        : serverHost(Config::SERVER_HOST)
        , serverPort(Config::SERVER_PORT)
        , connectTimeout(Config::CONNECT_TIMEOUT)
        , reconnectDelay(Config::RECONNECT_DELAY)
        , maxReconnectAttempts(Config::MAX_RECONNECT_ATTEMPTS)
        , clientSocket(INVALID_SOCKET)
        , connectionState(ConnectionState::DISCONNECTED)
        , shouldStop(false)
        , isRunning(false)
        , heartbeatSequence(0)
        , heartbeatInterval(Config::HEARTBEAT_INTERVAL)
        , bytesSent(0)
        , bytesReceived(0)
        , messagesSent(0)
        , messagesReceived(0)
    {
        LOG_DEBUG("NetworkManager initialized");
    }
    
    NetworkManager::~NetworkManager() {
        Stop();
        CleanupWinsock();
        LOG_DEBUG("NetworkManager destroyed");
    }
    
    bool NetworkManager::InitializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            LOG_ERROR("WSAStartup failed: " + std::to_string(result));
            return false;
        }
        LOG_DEBUG("Winsock initialized successfully");
#endif
        return true;
    }
    
    void NetworkManager::CleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
        LOG_DEBUG("Winsock cleaned up");
#endif
    }
    
    void NetworkManager::SetServerAddress(const std::string& host, uint16_t port) {
        serverHost = host;
        serverPort = port;
        LOG_INFO("Server address set to " + host + ":" + std::to_string(port));
    }
    
    void NetworkManager::SetConnectionTimeout(int timeoutSeconds) {
        connectTimeout = timeoutSeconds;
        LOG_DEBUG("Connection timeout set to " + std::to_string(timeoutSeconds) + " seconds");
    }
    
    void NetworkManager::SetReconnectSettings(int delaySeconds, int maxAttempts) {
        reconnectDelay = delaySeconds;
        maxReconnectAttempts = maxAttempts;
        LOG_DEBUG("Reconnect settings: delay=" + std::to_string(delaySeconds) + "s, max_attempts=" + std::to_string(maxAttempts));
    }
    
    void NetworkManager::SetHeartbeatInterval(int intervalSeconds) {
        heartbeatInterval = intervalSeconds;
        LOG_DEBUG("Heartbeat interval set to " + std::to_string(intervalSeconds) + " seconds");
    }
    
    void NetworkManager::SetMessageCallback(MessageCallback callback) {
        onMessageReceived = callback;
    }
    
    void NetworkManager::SetConnectionCallback(ConnectionCallback callback) {
        onConnectionStateChanged = callback;
    }
    
    void NetworkManager::SetErrorCallback(ErrorCallback callback) {
        onError = callback;
    }
    
    bool NetworkManager::Start() {
        if (isRunning) {
            LOG_WARNING("NetworkManager is already running");
            return true;
        }
        
        if (!InitializeWinsock()) {
            return false;
        }
        
        shouldStop = false;
        isRunning = true;
        ResetStatistics();
        
        // Start the connection thread
        connectionThread = std::thread(&NetworkManager::ConnectionThreadFunction, this);
        
        LOG_INFO("NetworkManager started");
        return true;
    }
    
    void NetworkManager::Stop() {
        if (!isRunning) {
            return;
        }
        
        LOG_INFO("Stopping NetworkManager...");
        shouldStop = true;
        
        // Close socket to interrupt blocking operations
        CloseSocket();
        
        // Notify send thread
        sendQueueCondition.notify_all();
        
        // Wait for threads to finish
        if (connectionThread.joinable()) {
            connectionThread.join();
        }
        if (sendThread.joinable()) {
            sendThread.join();
        }
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        if (heartbeatThread.joinable()) {
            heartbeatThread.join();
        }
        
        isRunning = false;
        SetConnectionState(ConnectionState::DISCONNECTED);
        
        LOG_INFO("NetworkManager stopped");
    }
    
    bool NetworkManager::IsConnected() const {
        return connectionState == ConnectionState::CONNECTED;
    }
    
    ConnectionState NetworkManager::GetConnectionState() const {
        return connectionState;
    }
    
    bool NetworkManager::CreateSocket() {
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            LOG_ERROR("Failed to create socket: " + GetLastSocketError());
            return false;
        }
        
        // Set socket options
#ifdef _WIN32
        DWORD timeout = 30000; // 30 seconds timeout
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = 30; // 30 seconds timeout
        tv.tv_usec = 0;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
        
        // Enable keep-alive
        int keepAlive = 1;
        setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
        
        // Set TCP_NODELAY to disable Nagle's algorithm for better responsiveness
        int noDelay = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
        LOG_DEBUG("Socket created successfully");
        return true;
    }
    
    void NetworkManager::CloseSocket() {
        if (IsSocketValid()) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            clientSocket = INVALID_SOCKET;
            LOG_DEBUG("Socket closed");
        }
    }
    
    bool NetworkManager::ConnectToServer() {
        if (!CreateSocket()) {
            return false;
        }
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        // Resolve hostname to IP address
        std::string resolvedIP = ResolveHostname(serverHost);
        if (resolvedIP.empty()) {
            LOG_ERROR("Failed to resolve hostname: " + serverHost);
            CloseSocket();
            return false;
        }
        
#ifdef _WIN32
        inet_pton(AF_INET, resolvedIP.c_str(), &serverAddr.sin_addr);
#else
        inet_aton(resolvedIP.c_str(), &serverAddr.sin_addr);
#endif
        
        LOG_INFO("Connecting to " + resolvedIP + ":" + std::to_string(serverPort));
        
        int result = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            LOG_ERROR("Failed to connect to server: " + GetLastSocketError());
            CloseSocket();
            return false;
        }
        
        connectionStartTime = std::chrono::steady_clock::now();
        LOG_INFO("Successfully connected to server");
        return true;
    }
    
    void NetworkManager::ConnectionThreadFunction() {
        int reconnectAttempts = 0;
        
        while (!shouldStop) {
            SetConnectionState(ConnectionState::CONNECTING);
            
            if (ConnectToServer()) {
                SetConnectionState(ConnectionState::CONNECTED);
                reconnectAttempts = 0;
                
                // Start send, receive, and heartbeat threads
                sendThread = std::thread(&NetworkManager::SendThreadFunction, this);
                receiveThread = std::thread(&NetworkManager::ReceiveThreadFunction, this);
                heartbeatThread = std::thread(&NetworkManager::HeartbeatThreadFunction, this);
                
                // Wait for threads to finish (connection lost or stop requested)
                if (sendThread.joinable()) sendThread.join();
                if (receiveThread.joinable()) receiveThread.join();
                if (heartbeatThread.joinable()) heartbeatThread.join();
                
                CloseSocket();
            }
            
            if (shouldStop) {
                break;
            }
            
            // Handle reconnection
            reconnectAttempts++;
            if (maxReconnectAttempts > 0 && reconnectAttempts >= maxReconnectAttempts) {
                LOG_ERROR("Maximum reconnection attempts reached");
                SetConnectionState(ConnectionState::ERROR_STATE);
                break;
            }
            
            SetConnectionState(ConnectionState::RECONNECTING);
            LOG_INFO("Reconnecting in " + std::to_string(reconnectDelay) + " seconds (attempt " + 
                    std::to_string(reconnectAttempts) + ")");
            
            // Wait for reconnect delay or stop signal
            std::this_thread::sleep_for(std::chrono::seconds(reconnectDelay));
        }
        
        SetConnectionState(ConnectionState::DISCONNECTED);
    }
    
    void NetworkManager::SendThreadFunction() {
        while (!shouldStop && IsSocketValid()) {
            std::unique_lock<std::mutex> lock(sendQueueMutex);
            
            // Wait for messages to send or stop signal
            sendQueueCondition.wait(lock, [this] { 
                return !sendQueue.empty() || shouldStop || !IsSocketValid(); 
            });
            
            if (shouldStop || !IsSocketValid()) {
                break;
            }
            
            while (!sendQueue.empty() && IsSocketValid()) {
                NetworkMessage message = sendQueue.front();
                sendQueue.pop();
                lock.unlock();
                
                // Send the message
                std::string data = message.data + "\n"; // Add delimiter
                int result = send(clientSocket, data.c_str(), static_cast<int>(data.length()), 0);
                
                if (result == SOCKET_ERROR) {
                    int error = 
#ifdef _WIN32
                        WSAGetLastError();
                    if (error != WSAETIMEDOUT && error != WSAEWOULDBLOCK) {
#else
                        errno;
                    if (error != EAGAIN && error != EWOULDBLOCK && error != ETIMEDOUT) {
#endif
                        LOG_ERROR("Failed to send message: " + GetLastSocketError());
                        HandleConnectionLost();
                        break;
                    } else {
                        // Timeout or would block - retry the message
                        sendQueue.push(message);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        lock.lock();
                        continue;
                    }
                } else {
                    bytesSent += result;
                    messagesSent++;
                    
                    if (!message.isHeartbeat) {
                        LOG_DEBUG("Message sent: " + std::to_string(result) + " bytes");
                    }
                }
                
                lock.lock();
            }
        }
    }
    
    void NetworkManager::ReceiveThreadFunction() {
        while (!shouldStop && IsSocketValid()) {
            int bytesReceived = recv(clientSocket, receiveBuffer, RECEIVE_BUFFER_SIZE - 1, 0);
            
            if (bytesReceived == SOCKET_ERROR) {
                int error = 
#ifdef _WIN32
                    WSAGetLastError();
                if (error != WSAETIMEDOUT && error != WSAEWOULDBLOCK) {
#else
                    errno;
                if (error != EAGAIN && error != EWOULDBLOCK && error != ETIMEDOUT) {
#endif
                    LOG_ERROR("Receive error: " + GetLastSocketError());
                    HandleConnectionLost();
                    break;
                } else {
                    // Timeout or would block - continue receiving
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            } else if (bytesReceived == 0) {
                LOG_INFO("Server closed the connection");
                HandleConnectionLost();
                break;
            } else {
                receiveBuffer[bytesReceived] = '\0';
                this->bytesReceived += bytesReceived;
                
                ProcessReceivedData(receiveBuffer, bytesReceived);
                lastHeartbeatReceived = std::chrono::steady_clock::now();
            }
        }
    }
    
    void NetworkManager::HeartbeatThreadFunction() {
        while (!shouldStop && IsSocketValid()) {
            std::this_thread::sleep_for(std::chrono::seconds(heartbeatInterval));
            
            if (shouldStop || !IsSocketValid()) {
                break;
            }
            
            // Send heartbeat
            if (!SendHeartbeat()) {
                LOG_WARNING("Failed to send heartbeat");
            }
            
            // Check for heartbeat timeout
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeatReceived);
            
            if (timeSinceLastHeartbeat.count() > heartbeatInterval * 3) {
                LOG_WARNING("Heartbeat timeout detected");
                HandleConnectionLost();
                break;
            }
        }
    }
    
    void NetworkManager::ProcessReceivedData(const char* data, size_t length) {
        incompleteMessage += std::string(data, length);
        
        // Process complete messages (delimited by newlines)
        size_t pos = 0;
        while ((pos = incompleteMessage.find('\n')) != std::string::npos) {
            std::string message = incompleteMessage.substr(0, pos);
            incompleteMessage.erase(0, pos + 1);
            
            if (!message.empty()) {
                messagesReceived++;
                
                // Check if it's a heartbeat response
                if (message.find("\"type\":\"heartbeat_response\"") != std::string::npos) {
                    LOG_DEBUG("Heartbeat response received");
                } else {
                    LOG_DEBUG("Message received: " + message);
                    
                    if (onMessageReceived) {
                        onMessageReceived(message);
                    }
                }
            }
        }
    }
    
    void NetworkManager::HandleConnectionLost() {
        LOG_WARNING("Connection lost");
        CloseSocket();
    }
    
    void NetworkManager::SetConnectionState(ConnectionState newState) {
        if (connectionState != newState) {
            connectionState = newState;
            
            std::string stateStr;
            switch (newState) {
                case ConnectionState::DISCONNECTED: stateStr = "DISCONNECTED"; break;
                case ConnectionState::CONNECTING: stateStr = "CONNECTING"; break;
                case ConnectionState::CONNECTED: stateStr = "CONNECTED"; break;
                case ConnectionState::RECONNECTING: stateStr = "RECONNECTING"; break;
                case ConnectionState::ERROR_STATE: stateStr = "ERROR"; break;
            }
            
            LOG_INFO("Connection state changed to: " + stateStr);
            
            if (onConnectionStateChanged) {
                onConnectionStateChanged(newState == ConnectionState::CONNECTED);
            }
        }
    }
    
    bool NetworkManager::IsSocketValid() const {
        return clientSocket != INVALID_SOCKET;
    }
    
    std::string NetworkManager::GetLastSocketError() const {
#ifdef _WIN32
        int error = WSAGetLastError();
        char* errorMsg = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      nullptr, error, 0, (LPSTR)&errorMsg, 0, nullptr);
        std::string result = errorMsg ? errorMsg : "Unknown error";
        if (errorMsg) LocalFree(errorMsg);
        return result + " (" + std::to_string(error) + ")";
#else
        return std::string(strerror(errno)) + " (" + std::to_string(errno) + ")";
#endif
    }
    
    void NetworkManager::ResetStatistics() {
        bytesSent = 0;
        bytesReceived = 0;
        messagesSent = 0;
        messagesReceived = 0;
        connectionStartTime = std::chrono::steady_clock::now();
    }
    
    bool NetworkManager::SendMessage(const std::string& message) {
        if (!IsConnected()) {
            LOG_WARNING("Cannot send message: not connected");
            return false;
        }
        
        QueueMessage(message, false);
        return true;
    }
    
    bool NetworkManager::SendHeartbeat() {
        if (!IsConnected()) {
            return false;
        }
        
        std::string heartbeatMsg = Protocol::CreateHeartbeatMessage("client_" + std::to_string(::GetCurrentProcessId()), ++heartbeatSequence);
        if (heartbeatMsg.empty()) {
            return false;
        }
        
        QueueMessage(heartbeatMsg, true);
        lastHeartbeatSent = std::chrono::steady_clock::now();
        return true;
    }
    
    void NetworkManager::QueueMessage(const std::string& message, bool isHeartbeat) {
        std::lock_guard<std::mutex> lock(sendQueueMutex);
        sendQueue.emplace(message, isHeartbeat);
        sendQueueCondition.notify_one();
    }
    
    // Statistics methods
    size_t NetworkManager::GetBytesSent() const {
        return bytesSent;
    }
    
    size_t NetworkManager::GetBytesReceived() const {
        return bytesReceived;
    }
    
    size_t NetworkManager::GetMessagesSent() const {
        return messagesSent;
    }
    
    size_t NetworkManager::GetMessagesReceived() const {
        return messagesReceived;
    }
    
    std::chrono::milliseconds NetworkManager::GetConnectionDuration() const {
        if (connectionState == ConnectionState::CONNECTED) {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - connectionStartTime);
        }
        return std::chrono::milliseconds(0);
    }
    
    double NetworkManager::GetConnectionUptime() const {
        // Implementation would track total uptime vs total runtime
        return IsConnected() ? 100.0 : 0.0;
    }
    
    std::string NetworkManager::GetServerAddress() const {
        return serverHost;
    }
    
    uint16_t NetworkManager::GetServerPort() const {
        return serverPort;
    }
    
    std::string NetworkManager::GetLocalAddress() const {
        if (!IsSocketValid()) {
            return "";
        }
        
        sockaddr_in localAddr;
        socklen_t addrLen = sizeof(localAddr);
        if (getsockname(clientSocket, (sockaddr*)&localAddr, &addrLen) == 0) {
            return inet_ntoa(localAddr.sin_addr);
        }
        return "";
    }
    
    uint16_t NetworkManager::GetLocalPort() const {
        if (!IsSocketValid()) {
            return 0;
        }
        
        sockaddr_in localAddr;
        socklen_t addrLen = sizeof(localAddr);
        if (getsockname(clientSocket, (sockaddr*)&localAddr, &addrLen) == 0) {
            return ntohs(localAddr.sin_port);
        }
        return 0;
    }
    
    bool NetworkManager::TestConnection(const std::string& host, uint16_t port, int timeoutMs) {
        socket_t testSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (testSocket == INVALID_SOCKET) {
            return false;
        }
        
        // Set timeout
#ifdef _WIN32
        DWORD timeout = timeoutMs;
        setsockopt(testSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(testSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        setsockopt(testSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(testSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        std::string resolvedIP = ResolveHostname(host);
        if (resolvedIP.empty()) {
#ifdef _WIN32
            closesocket(testSocket);
#else
            close(testSocket);
#endif
            return false;
        }
        
#ifdef _WIN32
        inet_pton(AF_INET, resolvedIP.c_str(), &addr.sin_addr);
#else
        inet_aton(resolvedIP.c_str(), &addr.sin_addr);
#endif
        
        bool result = (connect(testSocket, (sockaddr*)&addr, sizeof(addr)) == 0);
        
#ifdef _WIN32
        closesocket(testSocket);
#else
        close(testSocket);
#endif
        
        return result;
    }
    
    int NetworkManager::GetPing() {
        if (!IsConnected()) {
            return -1;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastHeartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeatSent);
        
        // Simple ping estimation based on heartbeat timing
        return static_cast<int>(timeSinceLastHeartbeat.count() / 2);
    }
    
    // Utility functions
    std::string GetLocalIPAddress() {
#ifdef _WIN32
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct hostent* host = gethostbyname(hostname);
            if (host != nullptr) {
                return inet_ntoa(*((struct in_addr*)host->h_addr_list[0]));
            }
        }
#else
        struct ifaddrs* ifaddrs_ptr;
        if (getifaddrs(&ifaddrs_ptr) == 0) {
            for (struct ifaddrs* ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    std::string ip = inet_ntoa(addr_in->sin_addr);
                    if (ip != "127.0.0.1") {
                        freeifaddrs(ifaddrs_ptr);
                        return ip;
                    }
                }
            }
            freeifaddrs(ifaddrs_ptr);
        }
#endif
        return "127.0.0.1";
    }
    
    std::vector<std::string> GetNetworkInterfaces() {
        std::vector<std::string> interfaces;
        
#ifdef _WIN32
        ULONG bufferSize = 0;
        GetAdaptersInfo(nullptr, &bufferSize);
        
        std::vector<char> buffer(bufferSize);
        PIP_ADAPTER_INFO adapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());
        
        if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
            for (PIP_ADAPTER_INFO adapter = adapterInfo; adapter != nullptr; adapter = adapter->Next) {
                interfaces.push_back(adapter->IpAddressList.IpAddress.String);
            }
        }
#else
        struct ifaddrs* ifaddrs_ptr;
        if (getifaddrs(&ifaddrs_ptr) == 0) {
            for (struct ifaddrs* ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    interfaces.push_back(inet_ntoa(addr_in->sin_addr));
                }
            }
            freeifaddrs(ifaddrs_ptr);
        }
#endif
        
        return interfaces;
    }
    
    bool IsPortOpen(const std::string& host, uint16_t port, int timeoutMs) {
        NetworkManager tempManager;
        return tempManager.TestConnection(host, port, timeoutMs);
    }
    
    std::string ResolveHostname(const std::string& hostname) {
        // Check if it's already an IP address
        sockaddr_in sa;
        int result = inet_pton(AF_INET, hostname.c_str(), &(sa.sin_addr));
        if (result == 1) {
            return hostname; // Already an IP address
        }
        
        // Resolve hostname
        struct hostent* host = gethostbyname(hostname.c_str());
        if (host == nullptr) {
            LOG_ERROR("Failed to resolve hostname: " + hostname);
            return "";
        }
        
        return inet_ntoa(*((struct in_addr*)host->h_addr_list[0]));
    }
}