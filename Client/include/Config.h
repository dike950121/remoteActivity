// Config.h - Handles client configuration loading and storage
#pragma once
#include <string>
using namespace std;

class Config {
public:
    // Load configuration from file or defaults
    static void load();
    // Get server address
    static std::string getServerAddress();
    // Get server port
    static int getServerPort();
}; 