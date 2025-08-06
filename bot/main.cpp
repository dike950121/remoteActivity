#include "bot_controller.h"
#include <iostream>

int main() {
    BotController bot("127.0.0.1", 5555);
    
    if (!bot.initialize()) {
        std::cerr << "Failed to initialize bot" << std::endl;
        return 1;
    }
    
    bot.run();
    
    return 0;
} 