// NetworkClient.cpp - Implementation of NetworkClient
#include "../include/NetworkClient.h"
#include "../include/CommandHandler.h"
#include <iostream>
#include <string>

NetworkClient::NetworkClient() : serverAddress("127.0.0.1"), serverPort(4444) {
    // TODO: Load server address/port from config
}

NetworkClient::~NetworkClient() {}

void NetworkClient::run() {
    // Main loop: connect and handle communication
    while (true) {
        if (connectToServer()) {
            handleServerCommunication();
        } else {
            reconnect();
        }
    }
}

bool NetworkClient::connectToServer() {
    // TODO: Implement TCP connection to server
    std::cout << "Connecting to server..." << std::endl;
    return true;
}

void NetworkClient::handleServerCommunication() {
    // TODO: Receive data, parse commands, and dispatch to CommandHandler
    std::cout << "Handling server communication..." << std::endl;
}

void NetworkClient::reconnect() {
    // TODO: Implement reconnection logic with backoff
    std::cout << "Reconnecting..." << std::endl;
} 