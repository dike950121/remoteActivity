#include "system_info.h"
#include <winsock2.h>
#include <iostream>
#include <cstdio>
#include <lmcons.h>

std::string SystemInfo::getSystemInformation() {
    std::string info = "=== SYSTEM INFORMATION ===\n";
    info += getBasicSystemInfo();
    info += getMemoryInfo();
    info += getTimeInfo();
    info += getNetworkInfo();
    info += getUserInfo();
    info += "========================\n";
    return info;
}

std::string SystemInfo::getBasicSystemInfo() {
    std::string info;
    
    // Get username
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len)) {
        info += "Username: " + std::string(username) + "\n";
    }
    
    // Get computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD computerName_len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA(computerName, &computerName_len)) {
        info += "Computer Name: " + std::string(computerName) + "\n";
    }
    
    // Get Windows version
    OSVERSIONINFOA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if (GetVersionExA(&osvi)) {
        info += "Windows Version: " + std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion) + "\n";
    }
    
    // Get current directory
    char currentDir[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, currentDir)) {
        info += "Current Directory: " + std::string(currentDir) + "\n";
    }
    
    // Get hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info += "Hostname: " + std::string(hostname) + "\n";
    }
    
    return info;
}

std::string SystemInfo::getMemoryInfo() {
    std::string info;
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info += "Total RAM: " + std::to_string(memInfo.ullTotalPhys / (1024 * 1024)) + " MB\n";
        info += "Available RAM: " + std::to_string(memInfo.ullAvailPhys / (1024 * 1024)) + " MB\n";
    }
    
    return info;
}

std::string SystemInfo::getTimeInfo() {
    std::string info;
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeStr[64];
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    info += "System Time: " + std::string(timeStr) + "\n";
    
    return info;
}

std::string SystemInfo::getNetworkInfo() {
    std::string info = "=== NETWORK INTERFACES ===\n";
    
    // Get local IP addresses using socket
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock != INVALID_SOCKET) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("8.8.8.8");
        addr.sin_port = htons(53);
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            int addr_len = sizeof(addr);
            if (getsockname(sock, (struct sockaddr*)&addr, &addr_len) == 0) {
                char localIP[16];
                strcpy(localIP, inet_ntoa(addr.sin_addr));
                info += "Primary IP: " + std::string(localIP) + "\n";
            }
        }
        closesocket(sock);
    }
    
    // Get additional network info
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host != NULL) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                struct in_addr addr;
                addr.s_addr = *(u_long*)host->h_addr_list[i];
                char ipStr[16];
                strcpy(ipStr, inet_ntoa(addr));
                info += "Network IP " + std::to_string(i+1) + ": " + std::string(ipStr) + "\n";
            }
        }
    }
    
    return info;
}

std::string SystemInfo::getUserInfo() {
    std::string info = "=== USER INFO ===\n";
    
    char userName[UNLEN + 1];
    DWORD userNameLen = UNLEN + 1;
    
    if (GetUserNameA(userName, &userNameLen)) {
        info += "Current User: " + std::string(userName) + "\n";
    }
    
    // Get session information
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
        info += "Session ID: " + std::to_string(sessionId) + "\n";
    }
    
    return info;
} 