// NetworkClient.h - Handles TCP connection and communication with the server
#pragma once
#include <string>
using namespace std;

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Main loop: connect to server, receive and dispatch commands
    void run();

private:
    // Establish connection to the server
    bool connectToServer();
    // Handle incoming data and dispatch commands
    void handleServerCommunication();
    // Reconnect logic
    void reconnect();
    // Server address and port
    std::string serverAddress;
    int serverPort;
}; 