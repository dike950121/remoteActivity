#include "network_client.h"
#include <iostream>

NetworkClient::NetworkClient(const char* ip, int port) 
    : sock(INVALID_SOCKET), server_ip((char*)ip), server_port(port), 
      isRunning(true), reconnectDelay(5000) {
}

NetworkClient::~NetworkClient() {
    disconnect();
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