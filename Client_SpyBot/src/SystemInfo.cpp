#include "SystemInfo.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <iphlpapi.h>
    #include <tlhelp32.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "psapi.lib")
#else
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <unistd.h>
    #include <fstream>
#endif

/**
 * @brief Default constructor
 */
SystemInfo::SystemInfo() {
}

/**
 * @brief Get operating system information
 * @return JSON formatted string with OS details
 */
std::string SystemInfo::GetOSInfo() {
    std::stringstream json;
    json << "{\"os_info\":{";
    
#ifdef _WIN32
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    json << "\"platform\":\"Windows\",";
    json << "\"version\":\"" << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "\",";
    json << "\"build\":\"" << osvi.dwBuildNumber << "\",";
    
    // Get computer name
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        json << "\"computer_name\":\"" << EscapeJsonString(computerName) << "\",";
    }
    
    // Get username
    char username[256];
    DWORD username_size = sizeof(username);
    if (GetUserNameA(username, &username_size)) {
        json << "\"username\":\"" << EscapeJsonString(username) << "\",";
    }
    
#else
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        json << "\"platform\":\"" << EscapeJsonString(unameData.sysname) << "\",";
        json << "\"version\":\"" << EscapeJsonString(unameData.release) << "\",";
        json << "\"architecture\":\"" << EscapeJsonString(unameData.machine) << "\",";
        json << "\"hostname\":\"" << EscapeJsonString(unameData.nodename) << "\",";
    }
    
    // Get username
    char* username = getenv("USER");
    if (username) {
        json << "\"username\":\"" << EscapeJsonString(username) << "\",";
    }
#endif
    
    // Add timestamp
    auto now = std::time(nullptr);
    json << "\"timestamp\":\"" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "\"";
    json << "}}";
    
    return json.str();
}

/**
 * @brief Get hardware information (CPU, Memory, Disk)
 * @return JSON formatted string with hardware details
 */
std::string SystemInfo::GetHardwareInfo() {
    std::stringstream json;
    json << "{\"hardware_info\":{";
    
#ifdef _WIN32
    // Get memory information
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        json << "\"total_memory\":" << memInfo.ullTotalPhys << ",";
        json << "\"available_memory\":" << memInfo.ullAvailPhys << ",";
        json << "\"memory_usage_percent\":" << memInfo.dwMemoryLoad << ",";
    }
    
    // Get system info
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    json << "\"processor_count\":" << sysInfo.dwNumberOfProcessors << ",";
    json << "\"processor_architecture\":" << sysInfo.wProcessorArchitecture << ",";
    
#else
    // Linux system information
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total_mem = 0, available_mem = 0;
    
    while (std::getline(meminfo, line)) {
        if (line.substr(0, 9) == "MemTotal:") {
            total_mem = std::stol(line.substr(10)) * 1024; // Convert to bytes
        } else if (line.substr(0, 13) == "MemAvailable:") {
            available_mem = std::stol(line.substr(14)) * 1024;
        }
    }
    
    json << "\"total_memory\":" << total_mem << ",";
    json << "\"available_memory\":" << available_mem << ",";
    
    // Get CPU count
    json << "\"processor_count\":" << sysconf(_SC_NPROCESSORS_ONLN) << ",";
#endif
    
    json << "\"timestamp\":\"" << std::time(nullptr) << "\"";
    json << "}}";
    
    return json.str();
}

/**
 * @brief Get list of currently running processes
 * @return Vector of ProcessInfo structures
 */
std::vector<ProcessInfo> SystemInfo::GetProcessList() {
    std::vector<ProcessInfo> processes;
    
#ifdef _WIN32
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            ProcessInfo proc;
            proc.pid = pe32.th32ProcessID;
            proc.name = pe32.szExeFile;
            
            // Try to get additional process information
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                // Get memory usage
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    proc.memory_usage = pmc.WorkingSetSize;
                }
                
                // Get process path
                char processPath[MAX_PATH];
                DWORD pathSize = sizeof(processPath);
                if (QueryFullProcessImageNameA(hProcess, 0, processPath, &pathSize)) {
                    proc.path = processPath;
                }
                
                CloseHandle(hProcess);
            }
            
            processes.push_back(proc);
            
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    
#else
    // Linux process enumeration
    system("ps -eo pid,comm,rss,pcpu --no-headers > /tmp/processes.txt");
    std::ifstream procFile("/tmp/processes.txt");
    std::string line;
    
    while (std::getline(procFile, line)) {
        std::istringstream iss(line);
        ProcessInfo proc;
        
        if (iss >> proc.pid >> proc.name >> proc.memory_usage >> proc.cpu_usage) {
            proc.memory_usage *= 1024; // Convert KB to bytes
            processes.push_back(proc);
        }
    }
    
    system("rm -f /tmp/processes.txt");
#endif
    
    return processes;
}

/**
 * @brief Get current network connections
 * @return Vector of NetworkConnection structures
 */
std::vector<NetworkConnection> SystemInfo::GetNetworkConnections() {
    std::vector<NetworkConnection> connections;
    
#ifdef _WIN32
    // Get TCP connections
    PMIB_TCPTABLE_OWNER_PID pTcpTable = NULL;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    
    // Make an initial call to GetExtendedTcpTable to get the size needed
    dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*) malloc(dwSize);
        if (pTcpTable == NULL) {
            return connections;
        }
    }
    
    // Make a second call to GetExtendedTcpTable to get the actual data
    dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwRetVal == NO_ERROR) {
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            NetworkConnection conn;
            conn.protocol = "TCP";
            
            // Convert addresses from network byte order
            struct in_addr localAddr;
            localAddr.s_addr = pTcpTable->table[i].dwLocalAddr;
            conn.local_address = inet_ntoa(localAddr);
            conn.local_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
            
            struct in_addr remoteAddr;
            remoteAddr.s_addr = pTcpTable->table[i].dwRemoteAddr;
            conn.remote_address = inet_ntoa(remoteAddr);
            conn.remote_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
            
            // Convert state
            switch (pTcpTable->table[i].dwState) {
                case MIB_TCP_STATE_LISTEN: conn.state = "LISTEN"; break;
                case MIB_TCP_STATE_ESTAB: conn.state = "ESTABLISHED"; break;
                case MIB_TCP_STATE_SYN_SENT: conn.state = "SYN_SENT"; break;
                case MIB_TCP_STATE_SYN_RCVD: conn.state = "SYN_RECEIVED"; break;
                case MIB_TCP_STATE_FIN_WAIT1: conn.state = "FIN_WAIT1"; break;
                case MIB_TCP_STATE_FIN_WAIT2: conn.state = "FIN_WAIT2"; break;
                case MIB_TCP_STATE_CLOSE_WAIT: conn.state = "CLOSE_WAIT"; break;
                case MIB_TCP_STATE_CLOSING: conn.state = "CLOSING"; break;
                case MIB_TCP_STATE_LAST_ACK: conn.state = "LAST_ACK"; break;
                case MIB_TCP_STATE_TIME_WAIT: conn.state = "TIME_WAIT"; break;
                case MIB_TCP_STATE_DELETE_TCB: conn.state = "DELETE_TCB"; break;
                default: conn.state = "UNKNOWN";
            }
            
            connections.push_back(conn);
        }
    }
    
    if (pTcpTable != NULL) {
        free(pTcpTable);
    }
    
#else
    // Linux network connections using netstat
    system("netstat -tulnp 2>/dev/null | grep -E '^(tcp|udp)' > /tmp/netstat.txt");
    std::ifstream netFile("/tmp/netstat.txt");
    std::string line;
    
    while (std::getline(netFile, line)) {
        std::istringstream iss(line);
        std::string proto, recv_q, send_q, local, remote, state;
        
        if (iss >> proto >> recv_q >> send_q >> local >> remote >> state) {
            NetworkConnection conn;
            conn.protocol = proto;
            conn.state = state;
            
            // Parse local address and port
            size_t colon = local.rfind(':');
            if (colon != std::string::npos) {
                conn.local_address = local.substr(0, colon);
                conn.local_port = std::stoi(local.substr(colon + 1));
            }
            
            // Parse remote address and port
            colon = remote.rfind(':');
            if (colon != std::string::npos) {
                conn.remote_address = remote.substr(0, colon);
                if (remote.substr(colon + 1) != "*") {
                    conn.remote_port = std::stoi(remote.substr(colon + 1));
                }
            }
            
            connections.push_back(conn);
        }
    }
    
    system("rm -f /tmp/netstat.txt");
#endif
    
    return connections;
}

/**
 * @brief Get current system performance metrics
 * @return JSON formatted string with performance data
 */
std::string SystemInfo::GetPerformanceMetrics() {
    std::stringstream json;
    json << "{\"performance\":{";
    
#ifdef _WIN32
    // Get CPU usage (simplified)
    static FILETIME prevSysKernel, prevSysUser, prevProcKernel, prevProcUser;
    static bool first_call = true;
    
    FILETIME sysIdle, sysKernel, sysUser, procCreation, procExit, procKernel, procUser;
    
    if (GetSystemTimes(&sysIdle, &sysKernel, &sysUser)) {
        json << "\"cpu_idle_time\":" << ((uint64_t)sysIdle.dwHighDateTime << 32) + sysIdle.dwLowDateTime << ",";
    }
    
    // Memory usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        json << "\"memory_usage_percent\":" << memInfo.dwMemoryLoad << ",";
        json << "\"total_memory\":" << memInfo.ullTotalPhys << ",";
        json << "\"available_memory\":" << memInfo.ullAvailPhys << ",";
    }
    
#else
    // Linux performance metrics
    std::ifstream loadavg("/proc/loadavg");
    std::string load1, load5, load15;
    if (loadavg >> load1 >> load5 >> load15) {
        json << "\"load_average_1min\":" << load1 << ",";
        json << "\"load_average_5min\":" << load5 << ",";
        json << "\"load_average_15min\":" << load15 << ",";
    }
    
    // Memory usage
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long total_mem = 0, available_mem = 0;
    
    while (std::getline(meminfo, line)) {
        if (line.substr(0, 9) == "MemTotal:") {
            total_mem = std::stol(line.substr(10));
        } else if (line.substr(0, 13) == "MemAvailable:") {
            available_mem = std::stol(line.substr(14));
        }
    }
    
    if (total_mem > 0) {
        int usage_percent = ((total_mem - available_mem) * 100) / total_mem;
        json << "\"memory_usage_percent\":" << usage_percent << ",";
        json << "\"total_memory\":" << (total_mem * 1024) << ",";
        json << "\"available_memory\":" << (available_mem * 1024) << ",";
    }
#endif
    
    json << "\"timestamp\":\"" << std::time(nullptr) << "\"";
    json << "}}";
    
    return json.str();
}

/**
 * @brief Get installed software list (Windows only)
 * @return JSON formatted string with software list
 */
std::string SystemInfo::GetInstalledSoftware() {
    // This is a simplified implementation - in production, you'd query the registry on Windows
    return "{\"installed_software\":{\"note\":\"Feature not implemented in this demo\",\"timestamp\":\"" + 
           std::to_string(std::time(nullptr)) + "\"}}";
}

/**
 * @brief Get current user information
 * @return JSON formatted string with user details
 */
std::string SystemInfo::GetUserInfo() {
    std::stringstream json;
    json << "{\"user_info\":{";
    
#ifdef _WIN32
    char username[256];
    DWORD username_size = sizeof(username);
    if (GetUserNameA(username, &username_size)) {
        json << "\"current_user\":\"" << EscapeJsonString(username) << "\",";
    }
    
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD comp_size = sizeof(computerName);
    if (GetComputerNameA(computerName, &comp_size)) {
        json << "\"computer_name\":\"" << EscapeJsonString(computerName) << "\",";
    }
#else
    char* username = getenv("USER");
    if (username) {
        json << "\"current_user\":\"" << EscapeJsonString(username) << "\",";
    }
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        json << "\"hostname\":\"" << EscapeJsonString(hostname) << "\",";
    }
#endif
    
    json << "\"timestamp\":\"" << std::time(nullptr) << "\"";
    json << "}}";
    
    return json.str();
}

/**
 * @brief Get network configuration and active interfaces
 * @return JSON formatted string with network configuration
 */
std::string SystemInfo::GetNetworkConfig() {
    std::stringstream json;
    json << "{\"network_config\":{";
    
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    
    pAdapterInfo = (IP_ADAPTER_INFO*) malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        json << "\"error\":\"Memory allocation failed\"";
        json << "}}";
        return json.str();
    }
    
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*) malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            json << "\"error\":\"Memory allocation failed\"";
            json << "}}";
            return json.str();
        }
    }
    
    dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (dwRetVal == NO_ERROR) {
        json << "\"adapters\":[";
        pAdapter = pAdapterInfo;
        bool first = true;
        
        while (pAdapter) {
            if (!first) json << ",";
            json << "{";
            json << "\"name\":\"" << EscapeJsonString(pAdapter->AdapterName) << "\",";
            json << "\"description\":\"" << EscapeJsonString(pAdapter->Description) << "\",";
            json << "\"ip_address\":\"" << pAdapter->IpAddressList.IpAddress.String << "\",";
            json << "\"subnet_mask\":\"" << pAdapter->IpAddressList.IpMask.String << "\",";
            json << "\"gateway\":\"" << pAdapter->GatewayList.IpAddress.String << "\"";
            json << "}";
            pAdapter = pAdapter->Next;
            first = false;
        }
        json << "],";
    }
    
    if (pAdapterInfo) {
        free(pAdapterInfo);
    }
    
#else
    json << "\"adapters\":[{\"note\":\"Linux network config collection not fully implemented\"}],";
#endif
    
    json << "\"timestamp\":\"" << std::time(nullptr) << "\"";
    json << "}}";
    
    return json.str();
}

/**
 * @brief Get comprehensive system report combining all information
 * @return JSON formatted string with complete system data
 */
std::string SystemInfo::GetCompleteSystemReport() {
    std::stringstream json;
    json << "{\"system_report\":{";
    json << "\"timestamp\":\"" << std::time(nullptr) << "\",";
    
    // Add OS information
    std::string osInfo = GetOSInfo();
    json << "\"os\":" << osInfo.substr(osInfo.find('{'), osInfo.rfind('}') - osInfo.find('{') + 1) << ",";
    
    // Add hardware information  
    std::string hwInfo = GetHardwareInfo();
    json << "\"hardware\":" << hwInfo.substr(hwInfo.find('{'), hwInfo.rfind('}') - hwInfo.find('{') + 1) << ",";
    
    // Add performance metrics
    std::string perfInfo = GetPerformanceMetrics();
    json << "\"performance\":" << perfInfo.substr(perfInfo.find('{'), perfInfo.rfind('}') - perfInfo.find('{') + 1) << ",";
    
    // Add processes
    std::vector<ProcessInfo> processes = GetProcessList();
    json << "\"processes\":" << ProcessListToJson(processes) << ",";
    
    // Add network connections
    std::vector<NetworkConnection> connections = GetNetworkConnections();
    json << "\"network_connections\":" << NetworkConnectionsToJson(connections);
    
    json << "}}";
    
    return json.str();
}

/**
 * @brief Convert ProcessInfo vector to JSON string
 * @param processes Vector of ProcessInfo structures
 * @return JSON formatted string
 */
std::string SystemInfo::ProcessListToJson(const std::vector<ProcessInfo>& processes) {
    std::stringstream json;
    json << "[";
    
    for (size_t i = 0; i < processes.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"pid\":" << processes[i].pid << ",";
        json << "\"name\":\"" << EscapeJsonString(processes[i].name) << "\",";
        json << "\"path\":\"" << EscapeJsonString(processes[i].path) << "\",";
        json << "\"memory_usage\":" << processes[i].memory_usage << ",";
        json << "\"cpu_usage\":" << processes[i].cpu_usage;
        json << "}";
        
        // Limit to first 50 processes to avoid too much data
        if (i >= 49) break;
    }
    
    json << "]";
    return json.str();
}

/**
 * @brief Convert NetworkConnection vector to JSON string
 * @param connections Vector of NetworkConnection structures
 * @return JSON formatted string
 */
std::string SystemInfo::NetworkConnectionsToJson(const std::vector<NetworkConnection>& connections) {
    std::stringstream json;
    json << "[";
    
    for (size_t i = 0; i < connections.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"protocol\":\"" << connections[i].protocol << "\",";
        json << "\"local_address\":\"" << connections[i].local_address << "\",";
        json << "\"local_port\":" << connections[i].local_port << ",";
        json << "\"remote_address\":\"" << connections[i].remote_address << "\",";
        json << "\"remote_port\":" << connections[i].remote_port << ",";
        json << "\"state\":\"" << connections[i].state << "\"";
        json << "}";
        
        // Limit to first 50 connections
        if (i >= 49) break;
    }
    
    json << "]";
    return json.str();
}

/**
 * @brief Escape special characters for JSON formatting
 * @param input String to escape
 * @return Escaped string safe for JSON
 */
std::string SystemInfo::EscapeJsonString(const std::string& input) {
    std::string output;
    output.reserve(input.length());
    
    for (char c : input) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    
    return output;
} 