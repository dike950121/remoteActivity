#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <winsock2.h>
#include <string>
#include <vector>

class NetworkClient {
private:
    WSADATA wsaData;
    SOCKET sock;
    SOCKET discoverySocket;
    struct sockaddr_in server;
    char* server_ip;
    int server_port;
    bool isRunning;
    int reconnectDelay;
    
    // Discovery related members
    std::vector<std::string> discoveredServers;
    bool discoveryMode;

public:
    NetworkClient(const char* ip = "127.0.0.1", int port = 5555);
    ~NetworkClient();
    
    bool initialize();
    bool connectToServer();
    void sendMessage(const char* message);
    void receiveResponse();
    void disconnect();
    bool isConnected() const;
    void setReconnectDelay(int delay);
    
    // Server discovery methods
    bool discoverServers(int timeoutSeconds = 10);
    std::string findBestServer();
    bool connectToDiscoveredServer();
    void setDiscoveryMode(bool enabled);
    std::vector<std::string> getDiscoveredServers() const;
    
    // Update methods
    bool handleUpdateCommand(const std::string& command);
    bool downloadUpdate(const std::string& updateUrl);
    void performSelfUpdate(const std::string& newExePath);
    
private:
    bool initializeDiscoverySocket();
    void cleanupDiscoverySocket();
    bool sendDiscoveryBroadcast();
    bool listenForServerResponses(int timeoutSeconds);
    std::string getLocalIP();
};

#endif // NETWORK_CLIENT_H 