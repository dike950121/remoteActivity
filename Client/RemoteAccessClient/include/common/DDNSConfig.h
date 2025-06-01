#pragma once

#include <string>
#include <vector>
#include "network/DDNSManager.h"

namespace Config {

    // DDNS Configuration Constants
    namespace DDNS {
        // Default update intervals (in seconds)
        constexpr int DEFAULT_UPDATE_INTERVAL = 300;    // 5 minutes
        constexpr int MIN_UPDATE_INTERVAL = 60;         // 1 minute
        constexpr int MAX_UPDATE_INTERVAL = 3600;       // 1 hour
        constexpr int EMERGENCY_UPDATE_INTERVAL = 30;   // 30 seconds for IP changes
        
        // Network timeouts
        constexpr int HTTP_TIMEOUT = 30;                // 30 seconds
        constexpr int CONNECTION_TIMEOUT = 15;          // 15 seconds
        constexpr int MAX_RETRIES = 3;
        
        // Provider priorities (lower = higher priority)
        constexpr int NO_IP_PRIORITY = 1;
        constexpr int DUCKDNS_PRIORITY = 2;
        constexpr int DYNU_PRIORITY = 3;
        constexpr int FREEDNS_PRIORITY = 4;
        
        // Default configurations for different providers
        inline Network::DDNSConfig GetDefaultNoIPConfig() {
            Network::DDNSConfig config;
            config.provider = Network::DDNSProvider::NO_IP;
            config.hostname = "";
            config.username = "";
            config.password = "";
            config.token = "";
            config.updateUrl = "http://dynupdate.no-ip.com/nic/update";
            config.updateInterval = DEFAULT_UPDATE_INTERVAL;
            config.enabled = false;
            config.priority = NO_IP_PRIORITY;
            return config;
        }
        
        inline Network::DDNSConfig GetDefaultDuckDNSConfig() {
            Network::DDNSConfig config;
            config.provider = Network::DDNSProvider::DUCKDNS;
            config.hostname = "";
            config.username = "";
            config.password = "";
            config.token = "";
            config.updateUrl = "https://www.duckdns.org/update";
            config.updateInterval = DEFAULT_UPDATE_INTERVAL;
            config.enabled = false;
            config.priority = DUCKDNS_PRIORITY;
            return config;
        }
        
        inline Network::DDNSConfig GetDefaultDynuConfig() {
            Network::DDNSConfig config;
            config.provider = Network::DDNSProvider::DYNU;
            config.hostname = "";
            config.username = "";
            config.password = "";
            config.token = "";
            config.updateUrl = "https://api.dynu.com/nic/update";
            config.updateInterval = DEFAULT_UPDATE_INTERVAL;
            config.enabled = false;
            config.priority = DYNU_PRIORITY;
            return config;
        }
        
        inline Network::DDNSConfig GetDefaultFreeDNSConfig() {
            Network::DDNSConfig config;
            config.provider = Network::DDNSProvider::FREEDNS;
            config.hostname = "";
            config.username = "";
            config.password = "";
            config.token = "";
            config.updateUrl = "https://freedns.afraid.org/dynamic/update.php";
            config.updateInterval = DEFAULT_UPDATE_INTERVAL;
            config.enabled = false;
            config.priority = FREEDNS_PRIORITY;
            return config;
        }
        
        // IP Detection Services
        inline std::vector<std::string> GetDefaultIPDetectionServices() {
            return {
                "http://checkip.amazonaws.com/",
                "http://ipv4.icanhazip.com/",
                "http://api.ipify.org/",
                "http://ipinfo.io/ip",
                "http://whatismyipaddress.com/api",
                "http://ip.42.pl/raw",
                "http://myexternalip.com/raw",
                "http://eth0.me/"
            };
        }
        
        // Fallback provider order
        inline std::vector<Network::DDNSProvider> GetDefaultFallbackOrder() {
            return {
                Network::DDNSProvider::NO_IP,
                Network::DDNSProvider::DUCKDNS,
                Network::DDNSProvider::DYNU,
                Network::DDNSProvider::FREEDNS
            };
        }
        
        // Provider-specific settings
        namespace NoIP {
            constexpr const char* UPDATE_URL = "http://dynupdate.no-ip.com/nic/update";
            constexpr const char* USER_AGENT = "RemoteAccessClient DDNS/1.0";
            constexpr int RATE_LIMIT_SECONDS = 300; // 5 minutes between updates
        }
        
        namespace DuckDNS {
            constexpr const char* UPDATE_URL = "https://www.duckdns.org/update";
            constexpr const char* USER_AGENT = "RemoteAccessClient DDNS/1.0";
            constexpr int RATE_LIMIT_SECONDS = 60; // 1 minute between updates
        }
        
        namespace Dynu {
            constexpr const char* UPDATE_URL = "https://api.dynu.com/nic/update";
            constexpr const char* USER_AGENT = "RemoteAccessClient DDNS/1.0";
            constexpr int RATE_LIMIT_SECONDS = 120; // 2 minutes between updates
        }
        
        namespace FreeDNS {
            constexpr const char* UPDATE_URL = "https://freedns.afraid.org/dynamic/update.php";
            constexpr const char* USER_AGENT = "RemoteAccessClient DDNS/1.0";
            constexpr int RATE_LIMIT_SECONDS = 300; // 5 minutes between updates
        }
    }
    
    // DDNS Manager Configuration
    class DDNSConfigManager {
    public:
        static DDNSConfigManager& GetInstance() {
            static DDNSConfigManager instance;
            return instance;
        }
        
        // Load configuration from file/registry
        bool LoadConfiguration();
        
        // Save configuration to file/registry
        bool SaveConfiguration();
        
        // Get all configured providers
        std::vector<Network::DDNSConfig> GetProviders() const;
        
        // Add or update provider configuration
        void SetProviderConfig(const Network::DDNSConfig& config);
        
        // Remove provider configuration
        void RemoveProvider(Network::DDNSProvider provider, const std::string& hostname);
        
        // Enable/disable DDNS functionality
        void SetEnabled(bool enabled);
        bool IsEnabled() const;
        
        // Fallback settings
        void SetFallbackEnabled(bool enabled);
        bool IsFallbackEnabled() const;
        
        void SetFallbackOrder(const std::vector<Network::DDNSProvider>& order);
        std::vector<Network::DDNSProvider> GetFallbackOrder() const;
        
        // IP detection settings
        void SetIPDetectionServices(const std::vector<std::string>& services);
        std::vector<std::string> GetIPDetectionServices() const;
        
    private:
        DDNSConfigManager() = default;
        ~DDNSConfigManager() = default;
        DDNSConfigManager(const DDNSConfigManager&) = delete;
        DDNSConfigManager& operator=(const DDNSConfigManager&) = delete;
        
        std::vector<Network::DDNSConfig> m_providers;
        bool m_enabled = false;
        bool m_fallbackEnabled = true;
        std::vector<Network::DDNSProvider> m_fallbackOrder;
        std::vector<std::string> m_ipDetectionServices;
        
        // Helper methods
        std::string GetConfigFilePath() const;
        std::string GetRegistryPath() const;
        bool LoadFromFile();
        bool SaveToFile();
        bool LoadFromRegistry();
        bool SaveToRegistry();
    };
    
} // namespace Config