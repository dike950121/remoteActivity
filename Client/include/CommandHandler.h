// CommandHandler.h - Parses and executes commands from the server
#pragma once
#include <string>
using namespace std;

class CommandHandler {
public:
    // Parse and execute a command received from the server
    static void handleCommand(const std::string& command);
}; 