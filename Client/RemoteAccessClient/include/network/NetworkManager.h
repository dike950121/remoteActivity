#pragma once

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace Network {
    
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        ERROR_STATE
    };
    
    struct NetworkMessage {
        std::string data;
        std::chrono::steady_clock::time_point timestamp;
        bool isHeartbeat;
        
        NetworkMessage(const std::string& msg, bool heartbeat = false)
            : data(msg), timestamp(std::chrono::steady_clock::now()), isHeartbeat(heartbeat) {}
    };
    
    class NetworkManager {
    public:
        // Callback function types
        using MessageCallback = std::function<void(const std::string&)>;
        using ConnectionCallback = std::function<void(bool)>; // true = connected, false = disconnected
        using ErrorCallback = std::function<void(const std::string&)>;
        
    private:
        // Connection settings
        std::string serverHost;
        uint16_t serverPort;
        int connectTimeout;
        int reconnectDelay;
        int maxReconnectAttempts;
        
        // Socket and connection state
        socket_t clientSocket;
        ConnectionState connectionState;
        std::atomic<bool> shouldStop;
        std::atomic<bool> isRunning;
        
        // Threading
        std::thread connectionThread;
        std::thread sendThread;
        std::thread receiveThread;
        std::thread heartbeatThread;
        
        // Message queues
        std::queue<NetworkMessage> sendQueue;
        std::mutex sendQueueMutex;
        std::condition_variable sendQueueCondition;
        
        // Callbacks
        MessageCallback onMessageReceived;
        ConnectionCallback onConnectionStateChanged;
        ErrorCallback onError;
        
        // Heartbeat management
        std::atomic<int64_t> heartbeatSequence;
        std::chrono::steady_clock::time_point lastHeartbeatSent;
        std::chrono::steady_clock::time_point lastHeartbeatReceived;
        int heartbeatInterval;
        
        // Statistics
        std::atomic<size_t> bytesSent;
        std::atomic<size_t> bytesReceived;
        std::atomic<size_t> messagesSent;
        std::atomic<size_t> messagesReceived;
        std::chrono::steady_clock::time_point connectionStartTime;
        
        // Buffer for receiving data
        static constexpr size_t RECEIVE_BUFFER_SIZE = 4096;
        char receiveBuffer[RECEIVE_BUFFER_SIZE];
        std::string incompleteMessage;
        
        // Private methods
        bool InitializeWinsock();
        void CleanupWinsock();
        bool CreateSocket();
        void CloseSocket();
        bool ConnectToServer();
        void ConnectionThreadFunction();
        void SendThreadFunction();
        void ReceiveThreadFunction();
        void HeartbeatThreadFunction();
        void ProcessReceivedData(const char* data, size_t length);
        void HandleConnectionLost();
        void SetConnectionState(ConnectionState newState);
        bool IsSocketValid() const;
        std::string GetLastSocketError() const;
        void ResetStatistics();
        
    public:
        NetworkManager();
        ~NetworkManager();
        
        // Configuration
        void SetServerAddress(const std::string& host, uint16_t port);
        void SetConnectionTimeout(int timeoutSeconds);
        void SetReconnectSettings(int delaySeconds, int maxAttempts);
        void SetHeartbeatInterval(int intervalSeconds);
        
        // Callback registration
        void SetMessageCallback(MessageCallback callback);
        void SetConnectionCallback(ConnectionCallback callback);
        void SetErrorCallback(ErrorCallback callback);
        
        // Connection management
        bool Start();
        void Stop();
        bool IsConnected() const;
        ConnectionState GetConnectionState() const;
        
        // Message sending
        bool SendMessage(const std::string& message);
        bool SendHeartbeat();
        void QueueMessage(const std::string& message, bool isHeartbeat = false);
        
        // Statistics
        size_t GetBytesSent() const;
        size_t GetBytesReceived() const;
        size_t GetMessagesSent() const;
        size_t GetMessagesReceived() const;
        std::chrono::milliseconds GetConnectionDuration() const;
        double GetConnectionUptime() const; // Returns uptime percentage
        
        // Utility methods
        std::string GetServerAddress() const;
        uint16_t GetServerPort() const;
        std::string GetLocalAddress() const;
        uint16_t GetLocalPort() const;
        
        // Network diagnostics
        bool TestConnection(const std::string& host, uint16_t port, int timeoutMs = 5000);
        int GetPing(); // Returns ping in milliseconds, -1 if not available
        
        // Delete copy constructor and assignment operator
        NetworkManager(const NetworkManager&) = delete;
        NetworkManager& operator=(const NetworkManager&) = delete;
    };
    
    // Utility functions
    std::string GetLocalIPAddress();
    std::vector<std::string> GetNetworkInterfaces();
    bool IsPortOpen(const std::string& host, uint16_t port, int timeoutMs = 3000);
    std::string ResolveHostname(const std::string& hostname);
}