#include "system/SystemInfo.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <wininet.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "wbemuuid.lib")
#endif


using namespace System;

// Constructor
SystemInfo::SystemInfo() : cacheTimeout(std::chrono::minutes(5)) {
#ifdef _WIN32
    perfCountersInitialized = false;
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddEnglishCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
    perfCountersInitialized = true;
#endif
}

// Destructor
SystemInfo::~SystemInfo() {
#ifdef _WIN32
    if (perfCountersInitialized) {
        PdhCloseQuery(cpuQuery);
    }
#endif
}

// Cache management
bool SystemInfo::IsCacheValid() const {
    auto now = std::chrono::system_clock::now();
    return (now - lastCacheUpdate) < cacheTimeout;
}

void SystemInfo::UpdateCache() const {
    lastCacheUpdate = std::chrono::system_clock::now();
}

// Basic system information
std::string SystemInfo::GetOperatingSystem() const {
#ifdef _WIN32
    return "Windows";
#else
    return "Linux";
#endif
}

std::string SystemInfo::GetOSVersion() const {
    if (!cachedOSVersion.empty() && IsCacheValid()) {
        return cachedOSVersion;
    }
    
#ifdef _WIN32
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        std::stringstream ss;
        ss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion 
           << " Build " << osvi.dwBuildNumber;
        cachedOSVersion = ss.str();
    }
#endif
    
    UpdateCache();
    return cachedOSVersion;
}

std::string SystemInfo::GetOSArchitecture() const {
#ifdef _WIN64
    return "x64";
#else
    return "x86";
#endif
}

std::string SystemInfo::GetComputerName() const {
    if (!cachedComputerName.empty() && IsCacheValid()) {
        return cachedComputerName;
    }
    
#ifdef _WIN32
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (::GetComputerNameA(computerName, &size)) {
        cachedComputerName = std::string(computerName);
    }
#endif
    
    UpdateCache();
    return cachedComputerName;
}

std::string SystemInfo::GetUserName() const {
    if (!cachedUserName.empty() && IsCacheValid()) {
        return cachedUserName;
    }
    
#ifdef _WIN32
    char userName[UNLEN + 1];
    DWORD size = sizeof(userName);
    if (::GetUserNameA(userName, &size)) {
        cachedUserName = std::string(userName);
    }
#endif
    
    UpdateCache();
    return cachedUserName;
}

std::string SystemInfo::GetDomainName() const {
#ifdef _WIN32
    char domainName[256];
    DWORD size = sizeof(domainName);
    if (GetComputerNameExA(ComputerNameDnsDomain, domainName, &size)) {
        return std::string(domainName);
    }
#endif
    return "";
}

// Hardware information
std::string SystemInfo::GetCPUInfo() const {
#ifdef _WIN32
    char cpuString[0x40];
    int cpuInfo[4] = {-1};
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    
    memset(cpuString, 0, sizeof(cpuString));
    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(cpuInfo, i);
        if (i == 0x80000002)
            memcpy(cpuString, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000003)
            memcpy(cpuString + 16, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000004)
            memcpy(cpuString + 32, cpuInfo, sizeof(cpuInfo));
    }
    return std::string(cpuString);
#endif
    return "Unknown";
}

uint32_t SystemInfo::GetCPUCores() const {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
#endif
    return 1;
}

uint64_t SystemInfo::GetTotalMemory() const {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys;
#endif
    return 0;
}

uint64_t SystemInfo::GetAvailableMemory() const {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
#endif
    return 0;
}

// Network information
std::vector<NetworkInterface> SystemInfo::GetNetworkInterfaces() const {
    if (!cachedNetworkInterfaces.empty() && IsCacheValid()) {
        return cachedNetworkInterfaces;
    }
    
    cachedNetworkInterfaces.clear();
    
#ifdef _WIN32
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG outBufLen = 0;
    
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufLen);
        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen) == NO_ERROR) {
            for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next) {
                NetworkInterface iface;
                
                // Convert wide string to string
                int len = WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, nullptr, 0, nullptr, nullptr);
                if (len > 0) {
                    std::vector<char> buffer(len);
                    WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, buffer.data(), len, nullptr, nullptr);
                    iface.name = std::string(buffer.data());
                }
                
                iface.description = pCurr->Description;
                iface.isUp = (pCurr->OperStatus == IfOperStatusUp);
                iface.isDhcpEnabled = (pCurr->Dhcpv4Enabled == 1);
                
                // MAC Address
                if (pCurr->PhysicalAddressLength > 0) {
                    std::stringstream ss;
                    for (DWORD i = 0; i < pCurr->PhysicalAddressLength; i++) {
                        if (i > 0) ss << ":";
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)pCurr->PhysicalAddress[i];
                    }
                    iface.macAddress = ss.str();
                }
                
                // IP Addresses
                for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast; pUnicast = pUnicast->Next) {
                    char ipStr[INET6_ADDRSTRLEN];
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        inet_ntop(AF_INET, &((struct sockaddr_in*)pUnicast->Address.lpSockaddr)->sin_addr, ipStr, INET_ADDRSTRLEN);
                        iface.ipAddresses.push_back(std::string(ipStr));
                    } else if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6) {
                        inet_ntop(AF_INET6, &((struct sockaddr_in6*)pUnicast->Address.lpSockaddr)->sin6_addr, ipStr, INET6_ADDRSTRLEN);
                        iface.ipAddresses.push_back(std::string(ipStr));
                    }
                }
                
                cachedNetworkInterfaces.push_back(iface);
            }
        }
        free(pAddresses);
    }
#endif
    
    UpdateCache();
    return cachedNetworkInterfaces;
}

// Process information
std::vector<ProcessInfo> SystemInfo::GetProcesses() const {
    std::vector<ProcessInfo> processes;
    
#ifdef _WIN32
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return processes;
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessInfo proc;
            proc.pid = pe32.th32ProcessID;
            proc.parentPid = pe32.th32ParentProcessID;
            proc.name = pe32.szExeFile;
            proc.threadCount = pe32.cntThreads;
            
            // Get additional process information
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    proc.memoryUsage = pmc.WorkingSetSize;
                    proc.workingSet = pmc.WorkingSetSize;
                    proc.virtualSize = pmc.PagefileUsage;
                }
                
                char processPath[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, NULL, processPath, MAX_PATH)) {
                    proc.path = std::string(processPath);
                }
                
                CloseHandle(hProcess);
            }
            
            processes.push_back(proc);
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
#endif
    
    return processes;
}

// Storage information
std::vector<DriveInfo> SystemInfo::GetDrives() const {
    if (!cachedDrives.empty() && IsCacheValid()) {
        return cachedDrives;
    }
    
    cachedDrives.clear();
    
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            DriveInfo drive;
            drive.letter = std::string(1, 'A' + i) + ":";
            
            ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
            if (GetDiskFreeSpaceExA(drive.letter.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                drive.totalSize = totalNumberOfBytes.QuadPart;
                drive.freeSpace = totalNumberOfFreeBytes.QuadPart;
                drive.usedSpace = drive.totalSize - drive.freeSpace;
                drive.usagePercentage = (double)drive.usedSpace / drive.totalSize * 100.0;
                drive.isReady = true;
            }
            
            UINT driveType = GetDriveTypeA(drive.letter.c_str());
            switch (driveType) {
                case DRIVE_FIXED: drive.driveType = "Fixed"; break;
                case DRIVE_REMOVABLE: drive.driveType = "Removable"; break;
                case DRIVE_REMOTE: drive.driveType = "Network"; break;
                case DRIVE_CDROM: drive.driveType = "CD-ROM"; break;
                case DRIVE_RAMDISK: drive.driveType = "RAM Disk"; break;
                default: drive.driveType = "Unknown"; break;
            }
            
            char volumeName[MAX_PATH + 1];
            char fileSystemName[MAX_PATH + 1];
            if (GetVolumeInformationA(drive.letter.c_str(), volumeName, sizeof(volumeName), NULL, NULL, NULL, fileSystemName, sizeof(fileSystemName))) {
                drive.label = std::string(volumeName);
                drive.fileSystem = std::string(fileSystemName);
            }
            
            cachedDrives.push_back(drive);
        }
    }
#endif
    
    UpdateCache();
    return cachedDrives;
}

// System metrics
SystemMetrics SystemInfo::GetSystemMetrics() const {
    SystemMetrics metrics;
    
#ifdef _WIN32
    // Memory information
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        metrics.totalMemory = memInfo.ullTotalPhys;
        metrics.availableMemory = memInfo.ullAvailPhys;
        metrics.usedMemory = metrics.totalMemory - metrics.availableMemory;
        metrics.memoryUsage = (double)metrics.usedMemory / metrics.totalMemory * 100.0;
    }
    
    // CPU usage
    if (perfCountersInitialized) {
        PDH_FMT_COUNTERVALUE counterVal;
        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
        metrics.cpuUsage = counterVal.doubleValue;
    }
    
    // Process and thread counts
    PERFORMANCE_INFORMATION perfInfo;
    if (GetPerformanceInfo(&perfInfo, sizeof(perfInfo))) {
        metrics.processCount = perfInfo.ProcessCount;
        metrics.handleCount = perfInfo.HandleCount;
    }
    
    // Boot time and uptime
    ULONGLONG tickCount = GetTickCount64();
    metrics.uptime = std::chrono::milliseconds(tickCount);
    
    auto now = std::chrono::system_clock::now();
    metrics.bootTime = now - metrics.uptime;
#endif
    
    return metrics;
}

// Security information
SecurityInfo SystemInfo::GetSecurityInfo() const {
    SecurityInfo secInfo;
    
#ifdef _WIN32
    // Check if running elevated
    secInfo.isElevated = IsProcessElevated();
    
    // Check UAC status
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD enableLUA = 0;
        DWORD dataSize = sizeof(enableLUA);
        if (RegQueryValueExA(hKey, "EnableLUA", NULL, NULL, (LPBYTE)&enableLUA, &dataSize) == ERROR_SUCCESS) {
            secInfo.uacEnabled = (enableLUA != 0);
        }
        RegCloseKey(hKey);
    }
    
    // Check for debugger
    secInfo.debuggerPresent = IsDebuggerPresent();
    
    // Detect virtual machine
    secInfo.vmType = DetectVirtualMachine();
    secInfo.isVirtualMachine = !secInfo.vmType.empty();
#endif
    
    return secInfo;
}

// Helper methods
bool SystemInfo::IsProcessElevated() const {
#ifdef _WIN32
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    return isElevated;
#endif
    return false;
}

std::string SystemInfo::DetectVirtualMachine() const {
#ifdef _WIN32
    // Check registry for VM indicators
    std::vector<std::pair<std::string, std::string>> vmChecks = {
        {"HARDWARE\\DESCRIPTION\\System\\BIOS", "SystemManufacturer"},
        {"HARDWARE\\DESCRIPTION\\System\\BIOS", "SystemProductName"},
        {"HARDWARE\\DESCRIPTION\\System\\BIOS", "VideoBiosVersion"}
    };
    
    for (const auto& check : vmChecks) {
        std::string value = GetRegistryValue(check.first, check.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        
        if (value.find("vmware") != std::string::npos) return "VMware";
        if (value.find("virtualbox") != std::string::npos) return "VirtualBox";
        if (value.find("microsoft corporation") != std::string::npos && value.find("virtual") != std::string::npos) return "Hyper-V";
        if (value.find("qemu") != std::string::npos) return "QEMU";
        if (value.find("xen") != std::string::npos) return "Xen";
    }
#endif
    return "";
}

std::string SystemInfo::GetRegistryValue(const std::string& keyPath, const std::string& valueName) const {
#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
#endif
    return "";
}

// Utility methods
std::string SystemInfo::FormatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
}

std::string SystemInfo::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

uint32_t SystemInfo::GetCurrentProcessId() {
#ifdef _WIN32
    return ::GetCurrentProcessId();
#endif
    return 0;
}

// Additional required methods with basic implementations
std::string SystemInfo::GetWorkgroup() const { return ""; }
std::string SystemInfo::GetTimeZone() const { return ""; }
std::string SystemInfo::GetLanguage() const { return ""; }
std::string SystemInfo::GetKeyboardLayout() const { return ""; }
uint32_t SystemInfo::GetCPUThreads() const { return GetCPUCores() * 2; }
std::string SystemInfo::GetCPUArchitecture() const { return GetOSArchitecture(); }
std::string SystemInfo::GetMotherboardInfo() const { return ""; }
std::string SystemInfo::GetBIOSInfo() const { return ""; }
std::vector<std::string> SystemInfo::GetGPUInfo() const { return {}; }
std::string SystemInfo::GetPublicIPAddress() const { return ""; }
std::string SystemInfo::GetLocalIPAddress() const { return ""; }
std::string SystemInfo::GetMACAddress() const { return ""; }
std::vector<std::string> SystemInfo::GetDNSServers() const { return {}; }
std::string SystemInfo::GetDefaultGateway() const { return ""; }
bool SystemInfo::IsInternetConnected() const { return false; }
uint64_t SystemInfo::GetTotalDiskSpace() const { return 0; }
uint64_t SystemInfo::GetFreeDiskSpace() const { return 0; }
ProcessInfo SystemInfo::GetProcessInfo(uint32_t pid) const { return ProcessInfo(); }
double SystemInfo::GetCPUUsage() const { return 0.0; }
double SystemInfo::GetMemoryUsage() const { return 0.0; }
std::chrono::milliseconds SystemInfo::GetUptime() const { return std::chrono::milliseconds(0); }
std::chrono::system_clock::time_point SystemInfo::GetBootTime() const { return std::chrono::system_clock::now(); }
UserInfo SystemInfo::GetCurrentUser() const { return UserInfo(); }
std::vector<UserInfo> SystemInfo::GetUsers() const { return {}; }
bool SystemInfo::IsAdministrator() const { return IsProcessElevated(); }
bool SystemInfo::IsVirtualMachine() const { return !DetectVirtualMachine().empty(); }
bool SystemInfo::IsDebuggerPresent() const { return ::IsDebuggerPresent(); }
std::vector<std::string> SystemInfo::GetInstalledPrograms() const { return {}; }
std::vector<std::string> SystemInfo::GetRunningServices() const { return {}; }
std::vector<std::string> SystemInfo::GetStartupPrograms() const { return {}; }
std::vector<std::string> SystemInfo::GetEnvironmentVariables() const { return {}; }
std::string SystemInfo::GetEnvironmentVariable(const std::string& name) const { return ""; }
std::map<std::string, std::string> SystemInfo::GetSystemSettings() const { return {}; }
std::vector<std::string> SystemInfo::GetFirewallRules() const { return {}; }
std::vector<std::string> SystemInfo::GetNetworkShares() const { return {}; }
std::vector<std::string> SystemInfo::GetScheduledTasks() const { return {}; }
std::string SystemInfo::GetSystemInfoJSON() const { return ""; }
std::string SystemInfo::GetSystemInfoXML() const { return ""; }
std::string SystemInfo::GetSystemSummary() const { return ""; }
void SystemInfo::RefreshCache() { UpdateCache(); }
void SystemInfo::SetCacheTimeout(std::chrono::milliseconds timeout) { cacheTimeout = timeout; }
std::string SystemInfo::FormatDuration(std::chrono::milliseconds duration) { return ""; }
std::string SystemInfo::FormatPercentage(double percentage) { return ""; }
bool SystemInfo::IsValidIPAddress(const std::string& ip) { return false; }
uint32_t SystemInfo::GetCurrentThreadId() { return 0; }