#pragma once

#include "SystemInfo.h"
#include "NetworkClient.h"
#include <memory>
#include "mingw.thread.h"
#include <atomic>
#include <chrono>

/**
 * @brief DataCollector class manages the collection and transmission of system data
 * 
 * This class coordinates between SystemInfo and NetworkClient to periodically
 * collect system information and send it to the remote server.
 */
class DataCollector {
public:
    /**
     * @brief Constructor for DataCollector
     * @param network_client Shared pointer to NetworkClient instance
     */
    DataCollector(std::shared_ptr<NetworkClient> network_client);
    
    /**
     * @brief Destructor - ensures proper cleanup of resources
     */
    ~DataCollector();
    
    /**
     * @brief Start the data collection and transmission process
     * @param collection_interval Interval in seconds between collections
     * @return true if started successfully, false otherwise
     */
    bool StartCollection(int collection_interval = 30);
    
    /**
     * @brief Stop the data collection process
     */
    void StopCollection();
    
    /**
     * @brief Check if data collection is currently running
     * @return true if collecting, false otherwise
     */
    bool IsCollecting() const;
    
    /**
     * @brief Manually trigger a single data collection and transmission
     * @return true if successful, false otherwise
     */
    bool CollectAndSendData();
    
    /**
     * @brief Set the data collection interval
     * @param interval_seconds New interval in seconds
     */
    void SetCollectionInterval(int interval_seconds);
    
    /**
     * @brief Get the current collection interval
     * @return Current interval in seconds
     */
    int GetCollectionInterval() const;
    
    /**
     * @brief Enable or disable specific data collection features
     * @param collect_processes Enable/disable process collection
     * @param collect_network Enable/disable network collection
     * @param collect_performance Enable/disable performance collection
     */
    void ConfigureCollection(bool collect_processes, bool collect_network, bool collect_performance);

private:
    std::shared_ptr<NetworkClient> network_client_;
    std::unique_ptr<SystemInfo> system_info_;
    
    std::atomic<bool> collecting_;
    std::atomic<bool> should_stop_;
    std::thread collection_thread_;
    
    int collection_interval_; // in seconds
    
    // Collection flags
    bool collect_processes_;
    bool collect_network_;
    bool collect_performance_;
    
    /**
     * @brief Main collection loop running in separate thread
     */
    void CollectionLoop();
    
    /**
     * @brief Build complete data package for transmission
     * @return JSON formatted string with collected data
     */
    std::string BuildDataPackage();
    
    /**
     * @brief Add timestamp to data package
     * @param data_json JSON string to add timestamp to
     * @return JSON string with timestamp added
     */
    std::string AddTimestamp(const std::string& data_json);
    
    /**
     * @brief Generate unique client identifier
     * @return Unique client ID string
     */
    std::string GenerateClientId();
}; 