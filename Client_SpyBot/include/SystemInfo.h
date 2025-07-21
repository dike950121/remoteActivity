#pragma once

#include <string>
#include <vector>
#include <map>

/**
 * @brief Structure to hold process information
 */
struct ProcessInfo {
    std::string name;
    int pid;
    std::string path;
    double cpu_usage;
    size_t memory_usage;
};

/**
 * @brief Structure to hold network connection information
 */
struct NetworkConnection {
    std::string protocol;
    std::string local_address;
    int local_port;
    std::string remote_address;
    int remote_port;
    std::string state;
};

/**
 * @brief SystemInfo class for collecting comprehensive system information
 * 
 * This class provides functionality to gather various system metrics
 * including OS information, hardware details, running processes, and network connections.
 */
class SystemInfo {
public:
    /**
     * @brief Default constructor
     */
    SystemInfo();
    
    /**
     * @brief Get operating system information
     * @return JSON formatted string with OS details
     */
    std::string GetOSInfo();
    
    /**
     * @brief Get hardware information (CPU, Memory, Disk)
     * @return JSON formatted string with hardware details
     */
    std::string GetHardwareInfo();
    
    /**
     * @brief Get list of currently running processes
     * @return Vector of ProcessInfo structures
     */
    std::vector<ProcessInfo> GetProcessList();
    
    /**
     * @brief Get current network connections
     * @return Vector of NetworkConnection structures
     */
    std::vector<NetworkConnection> GetNetworkConnections();
    
    /**
     * @brief Get current system performance metrics
     * @return JSON formatted string with performance data
     */
    std::string GetPerformanceMetrics();
    
    /**
     * @brief Get installed software list (Windows only)
     * @return JSON formatted string with software list
     */
    std::string GetInstalledSoftware();
    
    /**
     * @brief Get current user information
     * @return JSON formatted string with user details
     */
    std::string GetUserInfo();
    
    /**
     * @brief Get network configuration and active interfaces
     * @return JSON formatted string with network configuration
     */
    std::string GetNetworkConfig();
    
    /**
     * @brief Get comprehensive system report combining all information
     * @return JSON formatted string with complete system data
     */
    std::string GetCompleteSystemReport();

private:
    /**
     * @brief Convert ProcessInfo vector to JSON string
     * @param processes Vector of ProcessInfo structures
     * @return JSON formatted string
     */
    std::string ProcessListToJson(const std::vector<ProcessInfo>& processes);
    
    /**
     * @brief Convert NetworkConnection vector to JSON string
     * @param connections Vector of NetworkConnection structures
     * @return JSON formatted string
     */
    std::string NetworkConnectionsToJson(const std::vector<NetworkConnection>& connections);
    
    /**
     * @brief Escape special characters for JSON formatting
     * @param input String to escape
     * @return Escaped string safe for JSON
     */
    std::string EscapeJsonString(const std::string& input);
}; 