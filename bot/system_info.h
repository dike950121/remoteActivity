#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>
#include <windows.h>

class SystemInfo {
public:
    static std::string getSystemInformation();
    
private:
    static std::string getBasicSystemInfo();
    static std::string getNetworkInfo();
    static std::string getUserInfo();
    static std::string getMemoryInfo();
    static std::string getTimeInfo();
};

#endif // SYSTEM_INFO_H 