#pragma once

#include <string>
#include <map>

/**
 * @brief Configuration manager class for handling application settings
 * 
 * This class manages configuration settings loaded from JSON file
 * and provides easy access to configuration parameters.
 */
class ConfigManager {
public:
    /**
     * @brief Get singleton instance of ConfigManager
     * @return Reference to ConfigManager instance
     */
    static ConfigManager& GetInstance();
    
    /**
     * @brief Load configuration from JSON file
     * @param config_file Path to configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool LoadConfig(const std::string& config_file = "config.json");
    
    /**
     * @brief Get server IP address from configuration
     * @return Server IP address string
     */
    std::string GetServerIP() const;
    
    /**
     * @brief Get server port from configuration
     * @return Server port number
     */
    int GetServerPort() const;
    
    /**
     * @brief Get data collection interval from configuration
     * @return Collection interval in seconds
     */
    int GetCollectionInterval() const;
    
    /**
     * @brief Get client authentication token
     * @return Authentication token string
     */
    std::string GetAuthToken() const;
    
    /**
     * @brief Check if process collection is enabled
     * @return true if enabled, false otherwise
     */
    bool IsProcessCollectionEnabled() const;
    
    /**
     * @brief Check if network collection is enabled
     * @return true if enabled, false otherwise
     */
    bool IsNetworkCollectionEnabled() const;
    
    /**
     * @brief Check if performance collection is enabled
     * @return true if enabled, false otherwise
     */
    bool IsPerformanceCollectionEnabled() const;
    
    /**
     * @brief Get connection retry attempts
     * @return Number of retry attempts
     */
    int GetRetryAttempts() const;
    
    /**
     * @brief Get connection timeout in milliseconds
     * @return Timeout value in milliseconds
     */
    int GetConnectionTimeout() const;
    
    /**
     * @brief Check if debug logging is enabled
     * @return true if debug enabled, false otherwise
     */
    bool IsDebugEnabled() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    std::map<std::string, std::string> config_data_;
    bool config_loaded_;
    
    /**
     * @brief Parse JSON configuration string
     * @param json_content JSON string to parse
     * @return true if parsed successfully, false otherwise
     */
    bool ParseJsonConfig(const std::string& json_content);
    
    /**
     * @brief Get configuration value as string
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    std::string GetConfigValue(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief Get configuration value as integer
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value as integer
     */
    int GetConfigValueInt(const std::string& key, int default_value = 0) const;
    
         /**
      * @brief Get configuration value as boolean
      * @param key Configuration key
      * @param default_value Default value if key not found
      * @return Configuration value as boolean
      */
     bool GetConfigValueBool(const std::string& key, bool default_value = false) const;
     
     /**
      * @brief Parse a section of JSON configuration
      * @param section_content Content of the JSON section
      * @param prefix Prefix to add to keys
      */
     void ParseSection(const std::string& section_content, const std::string& prefix);
}; 