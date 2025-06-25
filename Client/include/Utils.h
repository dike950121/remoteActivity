// Utils.h - Utility functions for logging, error handling, etc.
#pragma once
#include <string>
using namespace std;

namespace Utils {
    // Log a message to console or file
    void log(const std::string& message);
    // Handle errors
    void handleError(const std::string& error);
} 