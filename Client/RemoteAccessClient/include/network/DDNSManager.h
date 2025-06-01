#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#endif

namespace Network {

    enum class DDNSProvider {
        NO_IP,
        DUCKDNS,
        DYNU,
        FREEDNS,
        CUSTOM
    };

    enum class DDNSStatus {
        SUCCESS,
        FAILED,
        NETWORK_ERROR,
        AUTH_ERROR,
        INVALID_HOSTNAME,
        RATE_LIMITED,
        UNKNOWN_ERROR
    };

    struct DDNSConfig {
        DDNSProvider provider;
        std::string hostname;
        std::string username;
        std::string password;
        std::string token;  // For token-based auth (DuckDNS)
        std::string updateUrl;
        int updateInterval; // in seconds
        bool enabled;
        int priority;       // Lower number = higher priority
    };

    struct DDNSResult {
        DDNSStatus status;
        std::string message;
        std::string currentIP;
        std::chrono::system_clock::time_point timestamp;
    };

    class DDNSProvider_Interface {
    public:
        virtual ~DDNSProvider_Interface() = default;
        virtual DDNSResult UpdateIP(const DDNSConfig& config, const std::string& newIP) = 0;
        virtual std::string GetProviderName() const = 0;
        virtual bool ValidateConfig(const DDNSConfig& config) const = 0;
    };

    class NoIPProvider : public DDNSProvider_Interface {
    public:
        DDNSResult UpdateIP(const DDNSConfig& config, const std::string& newIP) override;
        std::string GetProviderName() const override { return "NO-IP"; }
        bool ValidateConfig(const DDNSConfig& config) const override;
    };

    class DuckDNSProvider : public DDNSProvider_Interface {
    public:
        DDNSResult UpdateIP(const DDNSConfig& config, const std::string& newIP) override;
        std::string GetProviderName() const override { return "DuckDNS"; }
        bool ValidateConfig(const DDNSConfig& config) const override;
    };

    class DynuProvider : public DDNSProvider_Interface {
    public:
        DDNSResult UpdateIP(const DDNSConfig& config, const std::string& newIP) override;
        std::string GetProviderName() const override { return "Dynu"; }
        bool ValidateConfig(const DDNSConfig& config) const override;
    };

    class FreeDNSProvider : public DDNSProvider_Interface {
    public:
        DDNSResult UpdateIP(const DDNSConfig& config, const std::string& newIP) override;
        std::string GetProviderName() const override { return "FreeDNS"; }
        bool ValidateConfig(const DDNSConfig& config) const override;
    };

    class DDNSManager {
    public:
        using UpdateCallback = std::function<void(const DDNSResult&, const DDNSConfig&)>;
        using IPChangeCallback = std::function<void(const std::string& oldIP, const std::string& newIP)>;

        DDNSManager();
        ~DDNSManager();

        // Configuration management
        void AddProvider(const DDNSConfig& config);
        void RemoveProvider(DDNSProvider provider, const std::string& hostname);
        void UpdateProviderConfig(const DDNSConfig& config);
        std::vector<DDNSConfig> GetProviders() const;

        // Control methods
        bool Start();
        void Stop();
        bool IsRunning() const;

        // Manual update
        DDNSResult UpdateNow(DDNSProvider provider = DDNSProvider::NO_IP);
        DDNSResult UpdateAll();

        // IP detection
        std::string GetCurrentPublicIP();
        std::string GetLastKnownIP() const;

        // Callbacks
        void SetUpdateCallback(UpdateCallback callback);
        void SetIPChangeCallback(IPChangeCallback callback);

        // Statistics
        int GetSuccessfulUpdates() const;
        int GetFailedUpdates() const;
        std::chrono::system_clock::time_point GetLastUpdateTime() const;

        // Fallback management
        void EnableFallback(bool enable);
        bool IsFallbackEnabled() const;
        void SetFallbackEnabled(bool enabled);
        void SetFallbackOrder(const std::vector<DDNSProvider>& order);
        
        // IP detection services
        void SetIPDetectionServices(const std::vector<std::string>& services);
        std::vector<std::string> GetIPDetectionServices() const;
        
        // Additional methods
        std::string GetCurrentIP() const;
        
        // Statistics structure
        struct DDNSStatistics {
            int successfulUpdates;
            int failedUpdates;
            std::chrono::system_clock::time_point lastUpdateTime;
            std::string lastKnownIP;
            bool isRunning;
        };
        
        DDNSStatistics GetStatistics() const;

    private:
        // Internal methods
        void WorkerThread();
        void CheckAndUpdateIP();
        DDNSResult UpdateWithProvider(const DDNSConfig& config, const std::string& newIP);
        std::string DetectPublicIP();
        std::unique_ptr<DDNSProvider_Interface> CreateProvider(DDNSProvider provider);
        std::string HttpGet(const std::string& url, const std::string& userAgent = "");
        std::string HttpPost(const std::string& url, const std::string& data, const std::string& headers = "");
        std::string Base64Encode(const std::string& input);
        bool IsValidIP(const std::string& ip);
        void LogResult(const DDNSResult& result, const DDNSConfig& config);

        // Member variables
        std::vector<DDNSConfig> m_providers;
        std::atomic<bool> m_running;
        std::atomic<bool> m_shouldStop;
        std::unique_ptr<std::thread> m_workerThread;
        std::string m_lastKnownIP;
        std::string m_currentIP;
        
        // Callbacks
        UpdateCallback m_updateCallback;
        IPChangeCallback m_ipChangeCallback;
        
        // Statistics
        std::atomic<int> m_successfulUpdates;
        std::atomic<int> m_failedUpdates;
        std::chrono::system_clock::time_point m_lastUpdateTime;
        
        // Fallback settings
        bool m_fallbackEnabled;
        std::vector<DDNSProvider> m_fallbackOrder;
        
        // IP detection services
        std::vector<std::string> m_ipDetectionServices;
        
        // Thread synchronization
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        
        // Constants
        static const int DEFAULT_UPDATE_INTERVAL = 300; // 5 minutes
        static const int MIN_UPDATE_INTERVAL = 60;      // 1 minute
        static const int MAX_UPDATE_INTERVAL = 3600;    // 1 hour
        static const int HTTP_TIMEOUT = 30;             // 30 seconds
    };

} // namespace Network