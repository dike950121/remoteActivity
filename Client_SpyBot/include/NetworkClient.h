#pragma once

#include <string>
#include "mingw.thread.h"
#include <atomic>
#include <functional>

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
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

/**
 * @brief Network client class for handling TCP communication with the server
 * 
 * This class manages the connection to the remote server and handles
 * data transmission for the spy bot client application.
 */
class NetworkClient {
public:
    /**
     * @brief Constructor for NetworkClient
     * @param server_ip Server IP address to connect to
     * @param server_port Server port number
     */
    NetworkClient(const std::string& server_ip, int server_port);
    
    /**
     * @brief Destructor - ensures proper cleanup of socket resources
     */
    ~NetworkClient();
    
    /**
     * @brief Initialize the network client and establish connection
     * @return true if initialization successful, false otherwise
     */
    bool Initialize();
    
    /**
     * @brief Connect to the remote server
     * @return true if connection successful, false otherwise
     */
    bool Connect();
    
    /**
     * @brief Send JSON data to the server
     * @param json_data JSON formatted string to send
     * @return true if data sent successfully, false otherwise
     */
    bool SendData(const std::string& json_data);
    
    /**
     * @brief Disconnect from the server and cleanup
     */
    void Disconnect();
    
    /**
     * @brief Check if client is currently connected
     * @return true if connected, false otherwise
     */
    bool IsConnected() const;
    
    /**
     * @brief Set callback function for received data
     * @param callback Function to call when data is received
     */
    void SetDataReceivedCallback(std::function<void(const std::string&)> callback);
    
    /**
     * @brief Start listening for incoming data from server
     */
    void StartListening();
    
    /**
     * @brief Stop listening for incoming data
     */
    void StopListening();

private:
    std::string server_ip_;
    int server_port_;
    socket_t client_socket_;
    std::atomic<bool> connected_;
    std::atomic<bool> listening_;
    std::thread listen_thread_;
    std::function<void(const std::string&)> data_callback_;
    
    /**
     * @brief Background thread function for listening to server responses
     */
    void ListenForData();
    
    /**
     * @brief Initialize Winsock on Windows platform
     * @return true if successful, false otherwise
     */
    bool InitializeWinsock();
    
    /**
     * @brief Cleanup Winsock on Windows platform
     */
    void CleanupWinsock();
}; 