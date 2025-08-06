#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <winsock2.h>
#include <string>

class NetworkClient {
private:
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char* server_ip;
    int server_port;
    bool isRunning;
    int reconnectDelay;

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
};

#endif // NETWORK_CLIENT_H 