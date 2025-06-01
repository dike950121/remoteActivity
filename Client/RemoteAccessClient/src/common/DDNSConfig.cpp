#include "common/DDNSConfig.h"
#include "common/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <comdef.h>
#pragma comment(lib, "shell32.lib")
#endif

namespace Config {

    bool DDNSConfigManager::LoadConfiguration() {
        Logger::LogManager::GetInstance().Info("Loading DDNS configuration");
        
        // Try to load from file first, then registry as fallback
        if (LoadFromFile()) {
            Logger::LogManager::GetInstance().Info("DDNS configuration loaded from file");
            return true;
        }
        
#ifdef _WIN32
        if (LoadFromRegistry()) {
            Logger::LogManager::GetInstance().Info("DDNS configuration loaded from registry");
            return true;
        }
#endif
        
        // If no configuration found, create default configuration
        Logger::LogManager::GetInstance().Info("No DDNS configuration found, creating defaults");
        
        // Initialize with default settings
        m_enabled = false;
        m_fallbackEnabled = true;
        m_fallbackOrder = DDNS::GetDefaultFallbackOrder();
        m_ipDetectionServices = DDNS::GetDefaultIPDetectionServices();
        
        // Save default configuration
        SaveConfiguration();
        
        return true;
    }
    
    bool DDNSConfigManager::SaveConfiguration() {
        Logger::LogManager::GetInstance().Info("Saving DDNS configuration");
        
        // Try to save to file first
        if (SaveToFile()) {
            Logger::LogManager::GetInstance().Info("DDNS configuration saved to file");
            return true;
        }
        
#ifdef _WIN32
        if (SaveToRegistry()) {
            Logger::LogManager::GetInstance().Info("DDNS configuration saved to registry");
            return true;
        }
#endif
        
        Logger::LogManager::GetInstance().Error("Failed to save DDNS configuration");
        return false;
    }
    
    std::vector<Network::DDNSConfig> DDNSConfigManager::GetProviders() const {
        return m_providers;
    }
    
    void DDNSConfigManager::SetProviderConfig(const Network::DDNSConfig& config) {
        // Remove existing configuration for same provider/hostname
        auto it = std::remove_if(m_providers.begin(), m_providers.end(),
            [&config](const Network::DDNSConfig& existing) {
                return existing.provider == config.provider && 
                       existing.hostname == config.hostname;
            });
        
        if (it != m_providers.end()) {
            m_providers.erase(it, m_providers.end());
        }
        
        // Add new configuration
        m_providers.push_back(config);
        
        // Sort by priority
        std::sort(m_providers.begin(), m_providers.end(),
            [](const Network::DDNSConfig& a, const Network::DDNSConfig& b) {
                return a.priority < b.priority;
            });
        
        Logger::LogManager::GetInstance().Info("Updated DDNS provider configuration for " + config.hostname);
    }
    
    void DDNSConfigManager::RemoveProvider(Network::DDNSProvider provider, const std::string& hostname) {
        auto it = std::remove_if(m_providers.begin(), m_providers.end(),
            [provider, &hostname](const Network::DDNSConfig& config) {
                return config.provider == provider && config.hostname == hostname;
            });
        
        if (it != m_providers.end()) {
            m_providers.erase(it, m_providers.end());
            Logger::LogManager::GetInstance().Info("Removed DDNS provider configuration for " + hostname);
        }
    }
    
    void DDNSConfigManager::SetEnabled(bool enabled) {
        m_enabled = enabled;
        Logger::LogManager::GetInstance().Info("DDNS " + std::string(enabled ? "enabled" : "disabled"));
    }
    
    bool DDNSConfigManager::IsEnabled() const {
        return m_enabled;
    }
    
    void DDNSConfigManager::SetFallbackEnabled(bool enabled) {
        m_fallbackEnabled = enabled;
        Logger::LogManager::GetInstance().Info("DDNS fallback " + std::string(enabled ? "enabled" : "disabled"));
    }
    
    bool DDNSConfigManager::IsFallbackEnabled() const {
        return m_fallbackEnabled;
    }
    
    void DDNSConfigManager::SetFallbackOrder(const std::vector<Network::DDNSProvider>& order) {
        m_fallbackOrder = order;
        Logger::LogManager::GetInstance().Info("Updated DDNS fallback order");
    }
    
    std::vector<Network::DDNSProvider> DDNSConfigManager::GetFallbackOrder() const {
        return m_fallbackOrder;
    }
    
    void DDNSConfigManager::SetIPDetectionServices(const std::vector<std::string>& services) {
        m_ipDetectionServices = services;
        Logger::LogManager::GetInstance().Info("Updated IP detection services list");
    }
    
    std::vector<std::string> DDNSConfigManager::GetIPDetectionServices() const {
        return m_ipDetectionServices;
    }
    
    std::string DDNSConfigManager::GetConfigFilePath() const {
#ifdef _WIN32
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
            std::string configDir = std::string(appDataPath) + "\\RemoteAccessClient";
            
            // Create directory if it doesn't exist
            std::filesystem::create_directories(configDir);
            
            return configDir + "\\ddns_config.json";
        }
        return "ddns_config.json"; // Fallback to current directory
#else
        return std::string(getenv("HOME")) + "/.remoteaccessclient/ddns_config.json";
#endif
    }
    
    std::string DDNSConfigManager::GetRegistryPath() const {
#ifdef _WIN32
        return "SOFTWARE\\RemoteAccessClient\\DDNS";
#else
        return "";
#endif
    }
    
    bool DDNSConfigManager::LoadFromFile() {
        try {
            std::string configPath = GetConfigFilePath();
            std::ifstream file(configPath);
            
            if (!file.is_open()) {
                return false;
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            
            if (content.empty()) {
                return false;
            }
            
            // Simple JSON-like parsing (for demonstration)
            // In a real implementation, you'd use a proper JSON library
            
            // Parse enabled flag
            size_t enabledPos = content.find("\"enabled\":");
            if (enabledPos != std::string::npos) {
                size_t valueStart = content.find(":", enabledPos) + 1;
                size_t valueEnd = content.find(",", valueStart);
                if (valueEnd == std::string::npos) valueEnd = content.find("}", valueStart);
                
                std::string value = content.substr(valueStart, valueEnd - valueStart);
                value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                m_enabled = (value == "true");
            }
            
            // Parse fallback enabled flag
            size_t fallbackPos = content.find("\"fallbackEnabled\":");
            if (fallbackPos != std::string::npos) {
                size_t valueStart = content.find(":", fallbackPos) + 1;
                size_t valueEnd = content.find(",", valueStart);
                if (valueEnd == std::string::npos) valueEnd = content.find("}", valueStart);
                
                std::string value = content.substr(valueStart, valueEnd - valueStart);
                value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                m_fallbackEnabled = (value == "true");
            }
            
            // For this demonstration, we'll use default values for other settings
            // In a real implementation, you'd parse the complete JSON structure
            m_fallbackOrder = DDNS::GetDefaultFallbackOrder();
            m_ipDetectionServices = DDNS::GetDefaultIPDetectionServices();
            
            return true;
        }
        catch (const std::exception& e) {
            Logger::LogManager::GetInstance().Error("Failed to load DDNS config from file: " + std::string(e.what()));
            return false;
        }
    }
    
    bool DDNSConfigManager::SaveToFile() {
        try {
            std::string configPath = GetConfigFilePath();
            std::ofstream file(configPath);
            
            if (!file.is_open()) {
                return false;
            }
            
            // Simple JSON-like format (for demonstration)
            // In a real implementation, you'd use a proper JSON library
            file << "{\n";
            file << "  \"enabled\": " << (m_enabled ? "true" : "false") << ",\n";
            file << "  \"fallbackEnabled\": " << (m_fallbackEnabled ? "true" : "false") << ",\n";
            file << "  \"providers\": [\n";
            
            for (size_t i = 0; i < m_providers.size(); ++i) {
                const auto& provider = m_providers[i];
                file << "    {\n";
                file << "      \"provider\": " << static_cast<int>(provider.provider) << ",\n";
                file << "      \"hostname\": \"" << provider.hostname << "\",\n";
                file << "      \"username\": \"" << provider.username << "\",\n";
                file << "      \"password\": \"" << provider.password << "\",\n";
                file << "      \"token\": \"" << provider.token << "\",\n";
                file << "      \"updateUrl\": \"" << provider.updateUrl << "\",\n";
                file << "      \"updateInterval\": " << provider.updateInterval << ",\n";
                file << "      \"enabled\": " << (provider.enabled ? "true" : "false") << ",\n";
                file << "      \"priority\": " << provider.priority << "\n";
                file << "    }";
                if (i < m_providers.size() - 1) file << ",";
                file << "\n";
            }
            
            file << "  ],\n";
            file << "  \"ipDetectionServices\": [\n";
            
            for (size_t i = 0; i < m_ipDetectionServices.size(); ++i) {
                file << "    \"" << m_ipDetectionServices[i] << "\"";
                if (i < m_ipDetectionServices.size() - 1) file << ",";
                file << "\n";
            }
            
            file << "  ]\n";
            file << "}\n";
            
            file.close();
            return true;
        }
        catch (const std::exception& e) {
            Logger::LogManager::GetInstance().Error("Failed to save DDNS config to file: " + std::string(e.what()));
            return false;
        }
    }
    
#ifdef _WIN32
    bool DDNSConfigManager::LoadFromRegistry() {
        try {
            HKEY hKey;
            std::string regPath = GetRegistryPath();
            
            if (RegOpenKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                return false;
            }
            
            // Read enabled flag
            DWORD enabled = 0;
            DWORD dataSize = sizeof(DWORD);
            if (RegQueryValueExA(hKey, "Enabled", NULL, NULL, (LPBYTE)&enabled, &dataSize) == ERROR_SUCCESS) {
                m_enabled = (enabled != 0);
            }
            
            // Read fallback enabled flag
            DWORD fallbackEnabled = 1;
            dataSize = sizeof(DWORD);
            if (RegQueryValueExA(hKey, "FallbackEnabled", NULL, NULL, (LPBYTE)&fallbackEnabled, &dataSize) == ERROR_SUCCESS) {
                m_fallbackEnabled = (fallbackEnabled != 0);
            }
            
            RegCloseKey(hKey);
            
            // Use default values for other settings
            m_fallbackOrder = DDNS::GetDefaultFallbackOrder();
            m_ipDetectionServices = DDNS::GetDefaultIPDetectionServices();
            
            return true;
        }
        catch (const std::exception& e) {
            Logger::LogManager::GetInstance().Error("Failed to load DDNS config from registry: " + std::string(e.what()));
            return false;
        }
    }
    
    bool DDNSConfigManager::SaveToRegistry() {
        try {
            HKEY hKey;
            std::string regPath = GetRegistryPath();
            
            if (RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
                return false;
            }
            
            // Save enabled flag
            DWORD enabled = m_enabled ? 1 : 0;
            RegSetValueExA(hKey, "Enabled", 0, REG_DWORD, (LPBYTE)&enabled, sizeof(DWORD));
            
            // Save fallback enabled flag
            DWORD fallbackEnabled = m_fallbackEnabled ? 1 : 0;
            RegSetValueExA(hKey, "FallbackEnabled", 0, REG_DWORD, (LPBYTE)&fallbackEnabled, sizeof(DWORD));
            
            RegCloseKey(hKey);
            return true;
        }
        catch (const std::exception& e) {
            Logger::LogManager::GetInstance().Error("Failed to save DDNS config to registry: " + std::string(e.what()));
            return false;
        }
    }
#else
    bool DDNSConfigManager::LoadFromRegistry() {
        return false; // Not supported on non-Windows platforms
    }
    
    bool DDNSConfigManager::SaveToRegistry() {
        return false; // Not supported on non-Windows platforms
    }
#endif
    
} // namespace Config