#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>
#include <windows.h>

class SystemInfo {
public:
    static std::string getSystemInformation();
    static std::string getVersion();
    static std::string getUpdateUrl();
    static bool checkForUpdates();
    static bool downloadAndUpdate(const std::string& updateUrl);
    
private:
    static const std::string VERSION;
    static const std::string UPDATE_SERVER_URL;
    static bool downloadFile(const std::string& url, const std::string& localPath);
    static std::string getBasicSystemInfo();
    static std::string getNetworkInfo();
    static std::string getUserInfo();
    static std::string getMemoryInfo();
    static std::string getTimeInfo();
};

#endif // SYSTEM_INFO_H 