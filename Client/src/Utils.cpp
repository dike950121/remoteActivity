// Utils.cpp - Implementation of utility functions
#include "../include/Utils.h"
#include <iostream>
#include <string>

namespace Utils {
    void log(const std::string& message) {
        // TODO: Implement logging to file or console
        std::cout << "[LOG] " << message << std::endl;
    }

    void handleError(const std::string& error) {
        // TODO: Implement error handling
        std::cerr << "[ERROR] " << error << std::endl;
    }
} 