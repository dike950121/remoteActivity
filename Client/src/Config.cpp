// Config.cpp - Implementation of Config
#include "../include/Config.h"
#include <iostream>
#include <string>

void Config::load() {
    // TODO: Load configuration from file or set defaults
    std::cout << "Loading configuration..." << std::endl;
}

std::string Config::getServerAddress() {
    // TODO: Return loaded server address
    return "127.0.0.1";
}

int Config::getServerPort() {
    // TODO: Return loaded server port
    return 4444;
} 