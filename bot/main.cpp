#include "bot_controller.h"
#include <iostream>

int main() {
    std::cout << "=== Remote Activity Bot ===" << std::endl;
    std::cout << "Starting UDP server discovery..." << std::endl;
    
    // Create bot with default settings (will be overridden by discovery)
    BotController bot("127.0.0.1", 5555);
    
    if (!bot.initialize()) {
        std::cerr << "Failed to initialize bot" << std::endl;
        return 1;
    }
    
    // Try to discover servers using UDP broadcast
    if (bot.discoverAndConnect()) {
        std::cout << "Successfully connected to discovered server!" << std::endl;
    } else {
        std::cout << "Discovery failed, using fallback to localhost..." << std::endl;
    }
    
    bot.run();
    
    return 0;
} 