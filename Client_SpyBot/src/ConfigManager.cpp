#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

/**
 * @brief Get singleton instance of ConfigManager
 * @return Reference to ConfigManager instance
 */
ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

/**
 * @brief Load configuration from JSON file
 * @param config_file Path to configuration file
 * @return true if loaded successfully, false otherwise
 */
bool ConfigManager::LoadConfig(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open config file: " << config_file << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string content = buffer.str();
    if (content.empty()) {
        std::cerr << "[ERROR] Config file is empty: " << config_file << std::endl;
        return false;
    }
    
    config_loaded_ = ParseJsonConfig(content);
    return config_loaded_;
}

/**
 * @brief Parse JSON configuration string (simple parser)
 * @param json_content JSON string to parse
 * @return true if parsed successfully, false otherwise
 */
bool ConfigManager::ParseJsonConfig(const std::string& json_content) {
    try {
        config_data_.clear();
        
        // Simple JSON parser for basic key-value pairs
        // Note: This is a basic parser, for production use a proper JSON library like nlohmann/json
        
        std::string content = json_content;
        
        // Remove whitespace and braces
        content.erase(std::remove_if(content.begin(), content.end(), ::isspace), content.end());
        
        // Find and parse server section
        size_t server_start = content.find("\"server\":{");
        if (server_start != std::string::npos) {
            size_t server_end = content.find("}", server_start);
            std::string server_section = content.substr(server_start + 10, server_end - server_start - 10);
            ParseSection(server_section, "server.");
        }
        
        // Find and parse collection section
        size_t collection_start = content.find("\"collection\":{");
        if (collection_start != std::string::npos) {
            size_t collection_end = content.find("}", collection_start);
            std::string collection_section = content.substr(collection_start + 14, collection_end - collection_start - 14);
            ParseSection(collection_section, "collection.");
        }
        
        // Find and parse network section
        size_t network_start = content.find("\"network\":{");
        if (network_start != std::string::npos) {
            size_t network_end = content.find("}", network_start);
            std::string network_section = content.substr(network_start + 11, network_end - network_start - 11);
            ParseSection(network_section, "network.");
        }
        
        // Find and parse logging section
        size_t logging_start = content.find("\"logging\":{");
        if (logging_start != std::string::npos) {
            size_t logging_end = content.find("}", logging_start);
            std::string logging_section = content.substr(logging_start + 11, logging_end - logging_start - 11);
            ParseSection(logging_section, "logging.");
        }
        
        // Find and parse client section
        size_t client_start = content.find("\"client\":{");
        if (client_start != std::string::npos) {
            size_t client_end = content.find("}", client_start);
            std::string client_section = content.substr(client_start + 10, client_end - client_start - 10);
            ParseSection(client_section, "client.");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse JSON config: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Parse a section of JSON configuration
 * @param section_content Content of the JSON section
 * @param prefix Prefix to add to keys
 */
void ConfigManager::ParseSection(const std::string& section_content, const std::string& prefix) {
    std::string content = section_content;
    
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        size_t key_start = pos + 1;
        size_t key_end = content.find("\"", key_start);
        if (key_end == std::string::npos) break;
        
        std::string key = content.substr(key_start, key_end - key_start);
        
        size_t colon_pos = content.find(":", key_end);
        if (colon_pos == std::string::npos) break;
        
        size_t value_start = colon_pos + 1;
        
        // Skip quotes if present
        if (content[value_start] == '\"') {
            value_start++;
            size_t value_end = content.find("\"", value_start);
            if (value_end == std::string::npos) break;
            
            std::string value = content.substr(value_start, value_end - value_start);
            config_data_[prefix + key] = value;
            pos = value_end + 1;
        } else {
            // Numeric or boolean value
            size_t value_end = content.find_first_of(",}", value_start);
            if (value_end == std::string::npos) break;
            
            std::string value = content.substr(value_start, value_end - value_start);
            config_data_[prefix + key] = value;
            pos = value_end;
        }
    }
}

/**
 * @brief Get server IP address from configuration
 * @return Server IP address string
 */
std::string ConfigManager::GetServerIP() const {
    return GetConfigValue("server.ip", "127.0.0.1");
}

/**
 * @brief Get server port from configuration
 * @return Server port number
 */
int ConfigManager::GetServerPort() const {
    return GetConfigValueInt("server.port", 8888);
}

/**
 * @brief Get data collection interval from configuration
 * @return Collection interval in seconds
 */
int ConfigManager::GetCollectionInterval() const {
    return GetConfigValueInt("collection.interval_seconds", 30);
}

/**
 * @brief Get client authentication token
 * @return Authentication token string
 */
std::string ConfigManager::GetAuthToken() const {
    return GetConfigValue("server.auth_token", "");
}

/**
 * @brief Check if process collection is enabled
 * @return true if enabled, false otherwise
 */
bool ConfigManager::IsProcessCollectionEnabled() const {
    return GetConfigValueBool("collection.collect_processes", true);
}

/**
 * @brief Check if network collection is enabled
 * @return true if enabled, false otherwise
 */
bool ConfigManager::IsNetworkCollectionEnabled() const {
    return GetConfigValueBool("collection.collect_network", true);
}

/**
 * @brief Check if performance collection is enabled
 * @return true if enabled, false otherwise
 */
bool ConfigManager::IsPerformanceCollectionEnabled() const {
    return GetConfigValueBool("collection.collect_performance", true);
}

/**
 * @brief Get connection retry attempts
 * @return Number of retry attempts
 */
int ConfigManager::GetRetryAttempts() const {
    return GetConfigValueInt("network.retry_attempts", 3);
}

/**
 * @brief Get connection timeout in milliseconds
 * @return Timeout value in milliseconds
 */
int ConfigManager::GetConnectionTimeout() const {
    return GetConfigValueInt("network.connection_timeout_ms", 5000);
}

/**
 * @brief Check if debug logging is enabled
 * @return true if debug enabled, false otherwise
 */
bool ConfigManager::IsDebugEnabled() const {
    return GetConfigValueBool("logging.debug_enabled", false);
}

/**
 * @brief Get configuration value as string
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value
 */
std::string ConfigManager::GetConfigValue(const std::string& key, const std::string& default_value) const {
    auto it = config_data_.find(key);
    return (it != config_data_.end()) ? it->second : default_value;
}

/**
 * @brief Get configuration value as integer
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value as integer
 */
int ConfigManager::GetConfigValueInt(const std::string& key, int default_value) const {
    std::string value = GetConfigValue(key);
    if (value.empty()) return default_value;
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

/**
 * @brief Get configuration value as boolean
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value as boolean
 */
bool ConfigManager::GetConfigValueBool(const std::string& key, bool default_value) const {
    std::string value = GetConfigValue(key);
    if (value.empty()) return default_value;
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1" || value == "yes");
} 