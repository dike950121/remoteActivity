#include "DataCollector.h"
#include <iostream>
#include <sstream>
#include <ctime>

/**
 * @brief Constructor for DataCollector
 * @param network_client Shared pointer to NetworkClient instance
 */
DataCollector::DataCollector(std::shared_ptr<NetworkClient> network_client)
    : network_client_(network_client), system_info_(std::make_unique<SystemInfo>()),
      collecting_(false), should_stop_(false), collection_interval_(30),
      collect_processes_(true), collect_network_(true), collect_performance_(true) {
}

/**
 * @brief Destructor - ensures proper cleanup of resources
 */
DataCollector::~DataCollector() {
    StopCollection();
}

/**
 * @brief Start the data collection and transmission process
 * @param collection_interval Interval in seconds between collections
 * @return true if started successfully, false otherwise
 */
bool DataCollector::StartCollection(int collection_interval) {
    if (collecting_) {
        std::cout << "[INFO] Data collection is already running" << std::endl;
        return true;
    }
    
    if (!network_client_ || !network_client_->IsConnected()) {
        std::cerr << "[ERROR] Network client is not connected" << std::endl;
        return false;
    }
    
    collection_interval_ = collection_interval;
    should_stop_ = false;
    collecting_ = true;
    
    collection_thread_ = std::thread(&DataCollector::CollectionLoop, this);
    
    std::cout << "[INFO] Data collection started with interval: " << collection_interval << " seconds" << std::endl;
    return true;
}

/**
 * @brief Stop the data collection process
 */
void DataCollector::StopCollection() {
    if (!collecting_) {
        return;
    }
    
    should_stop_ = true;
    collecting_ = false;
    
    if (collection_thread_.joinable()) {
        collection_thread_.join();
    }
    
    std::cout << "[INFO] Data collection stopped" << std::endl;
}

/**
 * @brief Check if data collection is currently running
 * @return true if collecting, false otherwise
 */
bool DataCollector::IsCollecting() const {
    return collecting_;
}

/**
 * @brief Manually trigger a single data collection and transmission
 * @return true if successful, false otherwise
 */
bool DataCollector::CollectAndSendData() {
    if (!network_client_ || !network_client_->IsConnected()) {
        std::cerr << "[ERROR] Network client is not connected" << std::endl;
        return false;
    }
    
    try {
        std::string data_package = BuildDataPackage();
        if (data_package.empty()) {
            std::cerr << "[ERROR] Failed to build data package" << std::endl;
            return false;
        }
        
        if (!network_client_->SendData(data_package)) {
            std::cerr << "[ERROR] Failed to send data package" << std::endl;
            return false;
        }
        
        std::cout << "[INFO] Data package sent successfully (" << data_package.length() << " bytes)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during data collection: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Set the data collection interval
 * @param interval_seconds New interval in seconds
 */
void DataCollector::SetCollectionInterval(int interval_seconds) {
    collection_interval_ = interval_seconds;
    std::cout << "[INFO] Collection interval updated to: " << interval_seconds << " seconds" << std::endl;
}

/**
 * @brief Get the current collection interval
 * @return Current interval in seconds
 */
int DataCollector::GetCollectionInterval() const {
    return collection_interval_;
}

/**
 * @brief Enable or disable specific data collection features
 * @param collect_processes Enable/disable process collection
 * @param collect_network Enable/disable network collection
 * @param collect_performance Enable/disable performance collection
 */
void DataCollector::ConfigureCollection(bool collect_processes, bool collect_network, bool collect_performance) {
    collect_processes_ = collect_processes;
    collect_network_ = collect_network;
    collect_performance_ = collect_performance;
    
    std::cout << "[INFO] Collection configuration updated - Processes: " << (collect_processes ? "ON" : "OFF")
              << ", Network: " << (collect_network ? "ON" : "OFF") 
              << ", Performance: " << (collect_performance ? "ON" : "OFF") << std::endl;
}

/**
 * @brief Main collection loop running in separate thread
 */
void DataCollector::CollectionLoop() {
    std::cout << "[INFO] Data collection loop started" << std::endl;
    
    while (!should_stop_ && collecting_) {
        try {
            // Collect and send data
            if (!CollectAndSendData()) {
                std::cerr << "[WARN] Failed to collect/send data, will retry..." << std::endl;
            }
            
            // Wait for the specified interval
            for (int i = 0; i < collection_interval_ && !should_stop_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception in collection loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before retrying
        }
    }
    
    std::cout << "[INFO] Data collection loop ended" << std::endl;
}

/**
 * @brief Build complete data package for transmission
 * @return JSON formatted string with collected data
 */
std::string DataCollector::BuildDataPackage() {
    std::stringstream json;
    json << "{";
    json << "\"client_id\":\"" << GenerateClientId() << "\",";
    json << "\"message_type\":\"system_data\",";
    json << "\"timestamp\":\"" << std::time(nullptr) << "\",";
    json << "\"data\":{";
    
    bool first_section = true;
    
    // Always include basic system information
    std::string osInfo = system_info_->GetOSInfo();
    if (!osInfo.empty()) {
        if (!first_section) json << ",";
        json << "\"os_info\":" << osInfo;
        first_section = false;
    }
    
    std::string hwInfo = system_info_->GetHardwareInfo();
    if (!hwInfo.empty()) {
        if (!first_section) json << ",";
        json << "\"hardware_info\":" << hwInfo;
        first_section = false;
    }
    
    // Include performance metrics if enabled
    if (collect_performance_) {
        std::string perfInfo = system_info_->GetPerformanceMetrics();
        if (!perfInfo.empty()) {
            if (!first_section) json << ",";
            json << "\"performance\":" << perfInfo;
            first_section = false;
        }
    }
    
    // Include process information if enabled
    if (collect_processes_) {
        try {
            std::vector<ProcessInfo> processes = system_info_->GetProcessList();
            if (!processes.empty()) {
                if (!first_section) json << ",";
                json << "\"processes\":[";
                
                // Limit to first 20 processes to keep data manageable
                size_t max_processes = std::min(processes.size(), size_t(20));
                for (size_t i = 0; i < max_processes; ++i) {
                    if (i > 0) json << ",";
                    json << "{";
                    json << "\"pid\":" << processes[i].pid << ",";
                    json << "\"name\":\"" << processes[i].name << "\",";
                    json << "\"memory_usage\":" << processes[i].memory_usage;
                    json << "}";
                }
                json << "]";
                first_section = false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[WARN] Failed to collect process information: " << e.what() << std::endl;
        }
    }
    
    // Include network information if enabled
    if (collect_network_) {
        try {
            std::vector<NetworkConnection> connections = system_info_->GetNetworkConnections();
            if (!connections.empty()) {
                if (!first_section) json << ",";
                json << "\"network_connections\":[";
                
                // Limit to first 10 connections
                size_t max_connections = std::min(connections.size(), size_t(10));
                for (size_t i = 0; i < max_connections; ++i) {
                    if (i > 0) json << ",";
                    json << "{";
                    json << "\"protocol\":\"" << connections[i].protocol << "\",";
                    json << "\"local_address\":\"" << connections[i].local_address << "\",";
                    json << "\"local_port\":" << connections[i].local_port << ",";
                    json << "\"state\":\"" << connections[i].state << "\"";
                    json << "}";
                }
                json << "]";
                first_section = false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[WARN] Failed to collect network information: " << e.what() << std::endl;
        }
    }
    
    json << "}"; // Close data section
    json << "}"; // Close main object
    
    return json.str();
}

/**
 * @brief Add timestamp to data package
 * @param data_json JSON string to add timestamp to
 * @return JSON string with timestamp added
 */
std::string DataCollector::AddTimestamp(const std::string& data_json) {
    // This function is implemented within BuildDataPackage
    return data_json;
}

/**
 * @brief Generate unique client identifier
 * @return Unique client ID string
 */
std::string DataCollector::GenerateClientId() {
    // Simple client ID generation based on timestamp and process ID
    // In production, you might want to use a more sophisticated approach
    std::stringstream client_id;
    
#ifdef _WIN32
    client_id << "WIN_" << GetCurrentProcessId() << "_" << std::time(nullptr);
#else
    client_id << "LIN_" << getpid() << "_" << std::time(nullptr);
#endif
    
    return client_id.str();
} 