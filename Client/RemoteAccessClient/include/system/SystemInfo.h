#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <winver.h>
#define HLOG _HLOG
#include <lm.h>
#undef HLOG
#include <shlobj.h>
#include <userenv.h>
#include <wtsapi32.h>
#include <securitybaseapi.h>
#include <aclapi.h>
#include <sddl.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <proc/readproc.h>
#endif

namespace System {
    
    // Forward declarations
    struct ProcessInfo;
    struct NetworkInterface;
    struct DriveInfo;
    struct SystemMetrics;
    struct UserInfo;
    struct SecurityInfo;
    
    // Process information structure
    struct ProcessInfo {
        uint32_t pid;
        uint32_t parentPid;
        std::string name;
        std::string path;
        std::string commandLine;
        std::string user;
        uint64_t memoryUsage;     // in bytes
        double cpuUsage;          // percentage
        uint32_t threadCount;
        std::chrono::system_clock::time_point startTime;
        bool isSystem;
        bool is64Bit;
        std::string status;       // Running, Suspended, etc.
        uint32_t handleCount;
        uint64_t workingSet;
        uint64_t virtualSize;
        
        ProcessInfo() : pid(0), parentPid(0), memoryUsage(0), cpuUsage(0.0), 
                       threadCount(0), isSystem(false), is64Bit(false), 
                       handleCount(0), workingSet(0), virtualSize(0) {}
    };
    
    // Network interface information
    struct NetworkInterface {
        std::string name;
        std::string description;
        std::string macAddress;
        std::vector<std::string> ipAddresses;
        std::vector<std::string> subnetMasks;
        std::vector<std::string> gateways;
        std::vector<std::string> dnsServers;
        bool isUp;
        bool isDhcpEnabled;
        uint64_t bytesReceived;
        uint64_t bytesSent;
        uint64_t packetsReceived;
        uint64_t packetsSent;
        uint32_t mtu;
        std::string interfaceType;  // Ethernet, WiFi, etc.
        
        NetworkInterface() : isUp(false), isDhcpEnabled(false), 
                           bytesReceived(0), bytesSent(0), 
                           packetsReceived(0), packetsSent(0), mtu(0) {}
    };
    
    // Drive/disk information
    struct DriveInfo {
        std::string letter;       // Windows: "C:", Linux: "/dev/sda1"
        std::string label;
        std::string fileSystem;  // NTFS, ext4, etc.
        std::string driveType;   // Fixed, Removable, Network, etc.
        uint64_t totalSize;
        uint64_t freeSpace;
        uint64_t usedSpace;
        double usagePercentage;
        bool isReady;
        std::string mountPoint;   // Linux mount point
        
        DriveInfo() : totalSize(0), freeSpace(0), usedSpace(0), 
                     usagePercentage(0.0), isReady(false) {}
    };
    
    // System performance metrics
    struct SystemMetrics {
        double cpuUsage;          // Overall CPU usage percentage
        uint64_t totalMemory;     // Total RAM in bytes
        uint64_t availableMemory; // Available RAM in bytes
        uint64_t usedMemory;      // Used RAM in bytes
        double memoryUsage;       // Memory usage percentage
        uint32_t processCount;
        uint32_t threadCount;
        uint32_t handleCount;
        double diskUsage;         // Overall disk usage percentage
        uint64_t networkBytesReceived;
        uint64_t networkBytesSent;
        std::chrono::system_clock::time_point bootTime;
        std::chrono::milliseconds uptime;
        double loadAverage[3];    // 1, 5, 15 minute load averages (Linux)
        
        SystemMetrics() : cpuUsage(0.0), totalMemory(0), availableMemory(0), 
                         usedMemory(0), memoryUsage(0.0), processCount(0), 
                         threadCount(0), handleCount(0), diskUsage(0.0), 
                         networkBytesReceived(0), networkBytesSent(0) {
            loadAverage[0] = loadAverage[1] = loadAverage[2] = 0.0;
        }
    };
    
    // User information
    struct UserInfo {
        std::string username;
        std::string fullName;
        std::string domain;
        std::string homeDirectory;
        std::string shell;
        uint32_t userId;
        uint32_t groupId;
        std::vector<std::string> groups;
        bool isAdmin;
        bool isActive;
        std::chrono::system_clock::time_point lastLogin;
        std::string sessionType;  // Console, RDP, etc.
        
        UserInfo() : userId(0), groupId(0), isAdmin(false), isActive(false) {}
    };
    
    // Security information
    struct SecurityInfo {
        bool isElevated;          // Running with admin privileges
        bool uacEnabled;          // UAC status (Windows)
        bool firewallEnabled;
        bool antivirusEnabled;
        bool defenderEnabled;     // Windows Defender
        std::vector<std::string> securityProducts; // Installed security software
        std::string integrityLevel; // Low, Medium, High, System
        bool isVirtualMachine;
        std::string vmType;       // VMware, VirtualBox, Hyper-V, etc.
        bool debuggerPresent;
        
        SecurityInfo() : isElevated(false), uacEnabled(false), firewallEnabled(false), 
                        antivirusEnabled(false), defenderEnabled(false), 
                        isVirtualMachine(false), debuggerPresent(false) {}
    };
    
    // Main SystemInfo class
    class SystemInfo {
    private:
        // Cached information
        mutable std::string cachedOSVersion;
        mutable std::string cachedComputerName;
        mutable std::string cachedUserName;
        mutable std::vector<NetworkInterface> cachedNetworkInterfaces;
        mutable std::vector<DriveInfo> cachedDrives;
        mutable std::chrono::system_clock::time_point lastCacheUpdate;
        mutable std::chrono::milliseconds cacheTimeout;
        
        // Performance counters (Windows)
#ifdef _WIN32
        PDH_HQUERY cpuQuery;
        PDH_HCOUNTER cpuTotal;
        bool perfCountersInitialized;
#endif
        
        // Helper methods
        bool IsCacheValid() const;
        void UpdateCache() const;
        std::string GetRegistryValue(const std::string& keyPath, const std::string& valueName) const;
        std::vector<std::string> GetInstalledSoftware() const;
        std::string DetectVirtualMachine() const;
        bool IsProcessElevated() const;
        
    public:
        SystemInfo();
        ~SystemInfo();
        
        // Basic system information
        std::string GetOperatingSystem() const;
        std::string GetOSVersion() const;
        std::string GetOSArchitecture() const;
        std::string GetComputerName() const;
        std::string GetUserName() const;
        std::string GetDomainName() const;
        std::string GetWorkgroup() const;
        std::string GetTimeZone() const;
        std::string GetLanguage() const;
        std::string GetKeyboardLayout() const;
        
        // Hardware information
        std::string GetCPUInfo() const;
        uint32_t GetCPUCores() const;
        uint32_t GetCPUThreads() const;
        std::string GetCPUArchitecture() const;
        uint64_t GetTotalMemory() const;
        uint64_t GetAvailableMemory() const;
        std::string GetMotherboardInfo() const;
        std::string GetBIOSInfo() const;
        std::vector<std::string> GetGPUInfo() const;
        
        // Network information
        std::vector<NetworkInterface> GetNetworkInterfaces() const;
        std::string GetPublicIPAddress() const;
        std::string GetLocalIPAddress() const;
        std::string GetMACAddress() const;
        std::vector<std::string> GetDNSServers() const;
        std::string GetDefaultGateway() const;
        bool IsInternetConnected() const;
        
        // Storage information
        std::vector<DriveInfo> GetDrives() const;
        uint64_t GetTotalDiskSpace() const;
        uint64_t GetFreeDiskSpace() const;
        
        // Process and performance information
        std::vector<ProcessInfo> GetProcesses() const;
        ProcessInfo GetProcessInfo(uint32_t pid) const;
        SystemMetrics GetSystemMetrics() const;
        double GetCPUUsage() const;
        double GetMemoryUsage() const;
        std::chrono::milliseconds GetUptime() const;
        std::chrono::system_clock::time_point GetBootTime() const;
        
        // User and security information
        UserInfo GetCurrentUser() const;
        std::vector<UserInfo> GetUsers() const;
        SecurityInfo GetSecurityInfo() const;
        bool IsAdministrator() const;
        bool IsVirtualMachine() const;
        bool IsDebuggerPresent() const;
        
        // Software information
        std::vector<std::string> GetInstalledPrograms() const;
        std::vector<std::string> GetRunningServices() const;
        std::vector<std::string> GetStartupPrograms() const;
        std::vector<std::string> GetEnvironmentVariables() const;
        std::string GetEnvironmentVariable(const std::string& name) const;
        
        // System configuration
        std::map<std::string, std::string> GetSystemSettings() const;
        std::vector<std::string> GetFirewallRules() const;
        std::vector<std::string> GetNetworkShares() const;
        std::vector<std::string> GetScheduledTasks() const;
        
        // Utility methods
        std::string GetSystemInfoJSON() const;
        std::string GetSystemInfoXML() const;
        std::string GetSystemSummary() const;
        void RefreshCache();
        void SetCacheTimeout(std::chrono::milliseconds timeout);
        
        // Static utility methods
        static std::string FormatBytes(uint64_t bytes);
        static std::string FormatDuration(std::chrono::milliseconds duration);
        static std::string FormatPercentage(double percentage);
        static bool IsValidIPAddress(const std::string& ip);
        static std::string GetCurrentTimestamp();
        static uint32_t GetCurrentProcessId();
        static uint32_t GetCurrentThreadId();
    };
    
    // Global utility functions
    std::string GetSystemFingerprint();
    std::string GetHardwareFingerprint();
    bool IsSystemCompromised();
    std::vector<std::string> GetSecurityThreats();
    std::string GetSystemHealth();
    
} // namespace System