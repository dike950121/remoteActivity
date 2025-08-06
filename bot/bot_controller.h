#ifndef BOT_CONTROLLER_H
#define BOT_CONTROLLER_H

#include "network_client.h"
#include "system_info.h"
#include <string>
#include <windows.h>

class BotController {
private:
    NetworkClient networkClient;
    bool isRunning;
    int messageCounter;
    int updateInterval;

public:
    BotController(const char* server_ip = "127.0.0.1", int server_port = 5555);
    ~BotController();
    
    bool initialize();
    void run();
    void stop();
    void setUpdateInterval(int seconds);
    
private:
    void sendSystemInfo();
    void sendStatusUpdate();
    void handleReconnection();
};

#endif // BOT_CONTROLLER_H 