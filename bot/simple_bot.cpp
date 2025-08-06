#include <winsock2.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

class PersistentBot {
private:
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    char* server_ip = (char*)"127.0.0.1";
    int server_port = 5555;
    bool isRunning = true;
    int reconnectDelay = 5000; // 5 seconds

public:
    bool initialize() {
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
            std::cerr << "ERROR: WSAStartup failed" << std::endl;
            return false;
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(server_port);
        server.sin_addr.s_addr = inet_addr(server_ip);

        return true;
    }

    bool connectToServer() {
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

    void sendMessage(const char* message) {
        if (sock == INVALID_SOCKET) return;

        if (send(sock, message, strlen(message), 0) == SOCKET_ERROR) {
            std::cerr << "ERROR: Failed to send message" << std::endl;
            closesocket(sock);
            sock = INVALID_SOCKET;
        } else {
            std::cout << "Message sent: " << message << std::endl;
        }
    }

    void receiveResponse() {
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

    void run() {
        std::cout << "=== Persistent TCP Bot Client ===" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        if (!initialize()) {
            return;
        }

        int messageCounter = 1;
        
        while (isRunning) {
            if (sock == INVALID_SOCKET) {
                if (!connectToServer()) {
                    std::cout << "Reconnecting in " << (reconnectDelay/1000) << " seconds..." << std::endl;
                    Sleep(reconnectDelay);
                    continue;
                }
            }

            // Send periodic message
            char message[256];
            sprintf(message, "Hello from bot! Message #%d", messageCounter++);
            sendMessage(message);

            // Receive response
            receiveResponse();

            // Wait before next message
            Sleep(10000); // 10 seconds
        }

        cleanup();
    }

    void stop() {
        isRunning = false;
    }

    void cleanup() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
        }
        WSACleanup();
        std::cout << "Bot client stopped." << std::endl;
    }
};

int main() {
    PersistentBot bot;
    bot.run();
    return 0;
} 