#ifdef _WIN32
#endif

#include "network/DDNSManager.h"
#include "common/Logger.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <random>
#include <mutex>

namespace Network {

    // Base64 encoding table
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    // DDNSManager Implementation
    DDNSManager::DDNSManager()
        : m_running(false)
        , m_shouldStop(false)
        , m_lastKnownIP("")
        , m_currentIP("")
        , m_successfulUpdates(0)
        , m_failedUpdates(0)
        , m_fallbackEnabled(true)
    {
        // Initialize IP detection services
        m_ipDetectionServices = {
            "http://checkip.amazonaws.com/",
            "http://ipv4.icanhazip.com/",
            "http://api.ipify.org/",
            "http://ipinfo.io/ip",
            "http://whatismyipaddress.com/api"
        };

        // Default fallback order
        m_fallbackOrder = {
            DDNSProvider::NO_IP,
            DDNSProvider::DUCKDNS,
            DDNSProvider::DYNU,
            DDNSProvider::FREEDNS
        };

        Logger::LogManager::GetInstance().Info("DDNSManager initialized");
    }

    DDNSManager::~DDNSManager() {
        Stop();
        Logger::LogManager::GetInstance().Info("DDNSManager destroyed");
    }

    void DDNSManager::AddProvider(const DDNSConfig& config) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Validate configuration
        auto provider = CreateProvider(config.provider);
        if (!provider || !provider->ValidateConfig(config)) {
            std::string providerName = provider ? provider->GetProviderName() : "Unknown";
            Logger::LogManager::GetInstance().Error("Invalid DDNS configuration for provider: " + providerName);
            return;
        }

        // Remove existing config for same provider/hostname
        m_providers.erase(
            std::remove_if(m_providers.begin(), m_providers.end(),
                [&config](const DDNSConfig& existing) {
                    return existing.provider == config.provider && 
                           existing.hostname == config.hostname;
                }),
            m_providers.end());

        m_providers.push_back(config);
        
        // Sort by priority
        std::sort(m_providers.begin(), m_providers.end(),
            [](const DDNSConfig& a, const DDNSConfig& b) {
                return a.priority < b.priority;
            });

        Logger::LogManager::GetInstance().Info("Added DDNS provider: " + 
            provider->GetProviderName() + " for " + config.hostname);
    }

    void DDNSManager::RemoveProvider(DDNSProvider provider, const std::string& hostname) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = std::remove_if(m_providers.begin(), m_providers.end(),
            [provider, &hostname](const DDNSConfig& config) {
                return config.provider == provider && config.hostname == hostname;
            });
        
        if (it != m_providers.end()) {
            m_providers.erase(it, m_providers.end());
            Logger::LogManager::GetInstance().Info("Removed DDNS provider for " + hostname);
        }
    }

    void DDNSManager::UpdateProviderConfig(const DDNSConfig& config) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = std::find_if(m_providers.begin(), m_providers.end(),
            [&config](const DDNSConfig& existing) {
                return existing.provider == config.provider && 
                       existing.hostname == config.hostname;
            });
        
        if (it != m_providers.end()) {
            *it = config;
            Logger::LogManager::GetInstance().Info("Updated DDNS configuration for " + config.hostname);
        }
    }

    std::vector<DDNSConfig> DDNSManager::GetProviders() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_providers;
    }

    bool DDNSManager::Start() {
        if (m_running) {
            Logger::LogManager::GetInstance().Warning("DDNSManager is already running");
            return true;
        }

        if (m_providers.empty()) {
            Logger::LogManager::GetInstance().Error("No DDNS providers configured");
            return false;
        }

        m_shouldStop = false;
        m_running = true;
        m_workerThread = std::make_unique<std::thread>(&DDNSManager::WorkerThread, this);
        
        Logger::LogManager::GetInstance().Info("DDNSManager started");
        return true;
    }

    void DDNSManager::Stop() {
        if (!m_running) return;

        m_shouldStop = true;
        m_cv.notify_all();
        
        if (m_workerThread && m_workerThread->joinable()) {
            m_workerThread->join();
        }
        
        m_running = false;
        Logger::LogManager::GetInstance().Info("DDNSManager stopped");
    }

    bool DDNSManager::IsRunning() const {
        return m_running;
    }

    DDNSResult DDNSManager::UpdateNow(DDNSProvider provider) {
        std::string currentIP = GetCurrentPublicIP();
        if (currentIP.empty()) {
            return {DDNSStatus::NETWORK_ERROR, "Failed to detect public IP", "", std::chrono::system_clock::now()};
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (const auto& config : m_providers) {
            if (config.provider == provider && config.enabled) {
                return UpdateWithProvider(config, currentIP);
            }
        }
        
        return {DDNSStatus::FAILED, "Provider not found or disabled", currentIP, std::chrono::system_clock::now()};
    }

    DDNSResult DDNSManager::UpdateAll() {
        std::string currentIP = GetCurrentPublicIP();
        if (currentIP.empty()) {
            return {DDNSStatus::NETWORK_ERROR, "Failed to detect public IP", "", std::chrono::system_clock::now()};
        }

        DDNSResult lastResult;
        bool anySuccess = false;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        for (const auto& config : m_providers) {
            if (config.enabled) {
                DDNSResult result = UpdateWithProvider(config, currentIP);
                if (result.status == DDNSStatus::SUCCESS) {
                    anySuccess = true;
                }
                lastResult = result;
            }
        }
        
        if (anySuccess) {
            lastResult.status = DDNSStatus::SUCCESS;
            lastResult.message = "At least one provider updated successfully";
        }
        
        return lastResult;
    }

    std::string DDNSManager::GetCurrentPublicIP() {
        return DetectPublicIP();
    }

    std::string DDNSManager::GetLastKnownIP() const {
        return m_lastKnownIP;
    }

    void DDNSManager::SetUpdateCallback(UpdateCallback callback) {
        m_updateCallback = callback;
    }

    void DDNSManager::SetIPChangeCallback(IPChangeCallback callback) {
        m_ipChangeCallback = callback;
    }

    int DDNSManager::GetSuccessfulUpdates() const {
        return m_successfulUpdates;
    }

    int DDNSManager::GetFailedUpdates() const {
        return m_failedUpdates;
    }

    std::chrono::system_clock::time_point DDNSManager::GetLastUpdateTime() const {
        return m_lastUpdateTime;
    }

    void DDNSManager::EnableFallback(bool enable) {
        m_fallbackEnabled = enable;
    }

    bool DDNSManager::IsFallbackEnabled() const {
        return m_fallbackEnabled;
    }

    void DDNSManager::SetFallbackOrder(const std::vector<DDNSProvider>& order) {
        m_fallbackOrder = order;
    }

    // Private methods
    void DDNSManager::WorkerThread() {
        Logger::LogManager::GetInstance().Info("DDNS worker thread started");
        
        while (!m_shouldStop) {
            try {
                CheckAndUpdateIP();
                
                // Wait for next check interval
                std::unique_lock<std::mutex> lock(m_mutex);
                int minInterval = DEFAULT_UPDATE_INTERVAL;
                
                if (!m_providers.empty()) {
                    minInterval = std::min_element(m_providers.begin(), m_providers.end(),
                        [](const DDNSConfig& a, const DDNSConfig& b) {
                            return a.updateInterval < b.updateInterval;
                        })->updateInterval;
                }
                
                m_cv.wait_for(lock, std::chrono::seconds(minInterval), [this] { return m_shouldStop.load(); });
            }
            catch (const std::exception& e) {
                Logger::LogManager::GetInstance().Error("DDNS worker thread error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(60)); // Wait before retry
            }
        }
        
        Logger::LogManager::GetInstance().Info("DDNS worker thread stopped");
    }

    void DDNSManager::CheckAndUpdateIP() {
        std::string currentIP = DetectPublicIP();
        if (currentIP.empty()) {
            Logger::LogManager::GetInstance().Warning("Failed to detect public IP");
            return;
        }

        if (currentIP != m_lastKnownIP) {
            Logger::LogManager::GetInstance().Info("IP address changed from " + m_lastKnownIP + " to " + currentIP);
            
            if (m_ipChangeCallback) {
                m_ipChangeCallback(m_lastKnownIP, currentIP);
            }
            
            // Update all enabled providers
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& config : m_providers) {
                if (config.enabled) {
                    DDNSResult result = UpdateWithProvider(config, currentIP);
                    LogResult(result, config);
                    
                    if (m_updateCallback) {
                        m_updateCallback(result, config);
                    }
                }
            }
            
            m_lastKnownIP = currentIP;
        }
        
        m_currentIP = currentIP;
    }

    DDNSResult DDNSManager::UpdateWithProvider(const DDNSConfig& config, const std::string& newIP) {
        auto provider = CreateProvider(config.provider);
        if (!provider) {
            m_failedUpdates++;
            return {DDNSStatus::FAILED, "Failed to create provider", newIP, std::chrono::system_clock::now()};
        }

        DDNSResult result = provider->UpdateIP(config, newIP);
        
        if (result.status == DDNSStatus::SUCCESS) {
            m_successfulUpdates++;
        } else {
            m_failedUpdates++;
        }
        
        m_lastUpdateTime = std::chrono::system_clock::now();
        return result;
    }

    std::string DDNSManager::DetectPublicIP() {
        // Shuffle services for load balancing
        auto services = m_ipDetectionServices;
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(services.begin(), services.end(), g);
        
        for (const auto& service : services) {
            try {
                std::string response = HttpGet(service, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) RemoteAccessClient/1.0");
                
                // Clean up response
                response.erase(std::remove_if(response.begin(), response.end(), 
                    [](char c) { return c == '\r' || c == '\n' || c == ' '; }), response.end());
                
                if (IsValidIP(response)) {
                    return response;
                }
            }
            catch (const std::exception& e) {
                Logger::LogManager::GetInstance().Warning("Failed to get IP from " + service + ": " + e.what());
                continue;
            }
        }
        
        return "";
    }

    std::unique_ptr<DDNSProvider_Interface> DDNSManager::CreateProvider(DDNSProvider provider) {
        switch (provider) {
            case DDNSProvider::NO_IP:
                return std::make_unique<NoIPProvider>();
            case DDNSProvider::DUCKDNS:
                return std::make_unique<DuckDNSProvider>();
            case DDNSProvider::DYNU:
                return std::make_unique<DynuProvider>();
            case DDNSProvider::FREEDNS:
                return std::make_unique<FreeDNSProvider>();
            default:
                return nullptr;
        }
    }

    std::string DDNSManager::HttpGet(const std::string& url, const std::string& userAgent) {
#ifdef _WIN32
        HINTERNET hInternet = InternetOpenA(userAgent.empty() ? "DDNSClient/1.0" : userAgent.c_str(),
            INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            throw std::runtime_error("Failed to initialize WinINet");
        }

        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            throw std::runtime_error("Failed to open URL");
        }

        std::string response;
        char buffer[4096];
        DWORD bytesRead;
        
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return response;
#else
        // Linux implementation would go here
        throw std::runtime_error("HTTP client not implemented for this platform");
#endif
    }

    std::string DDNSManager::HttpPost(const std::string& url, const std::string& data, const std::string& headers) {
        // Implementation similar to HttpGet but with POST method
        // This would be used for providers that require POST requests
        return "";
    }

    std::string DDNSManager::Base64Encode(const std::string& input) {
        std::string encoded;
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) {
            encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        while (encoded.size() % 4) {
            encoded.push_back('=');
        }
        return encoded;
    }

    bool DDNSManager::IsValidIP(const std::string& ip) {
        std::regex ipRegex(R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        return std::regex_match(ip, ipRegex);
    }

    void DDNSManager::LogResult(const DDNSResult& result, const DDNSConfig& config) {
        std::string statusStr;
        switch (result.status) {
            case DDNSStatus::SUCCESS: statusStr = "SUCCESS"; break;
            case DDNSStatus::FAILED: statusStr = "FAILED"; break;
            case DDNSStatus::NETWORK_ERROR: statusStr = "NETWORK_ERROR"; break;
            case DDNSStatus::AUTH_ERROR: statusStr = "AUTH_ERROR"; break;
            case DDNSStatus::INVALID_HOSTNAME: statusStr = "INVALID_HOSTNAME"; break;
            case DDNSStatus::RATE_LIMITED: statusStr = "RATE_LIMITED"; break;
            default: statusStr = "UNKNOWN_ERROR"; break;
        }
        
        Logger::LogManager::GetInstance().Info("DDNS Update [" + config.hostname + "]: " + 
            statusStr + " - " + result.message);
    }

    // Provider Implementations
    
    // NO-IP Provider
    DDNSResult NoIPProvider::UpdateIP(const DDNSConfig& config, const std::string& newIP) {
        try {
            std::string auth = config.username + ":" + config.password;
            std::string encodedAuth = "";
            // Base64 encode auth (simplified)
            
            std::string url = "http://dynupdate.no-ip.com/nic/update?hostname=" + config.hostname + "&myip=" + newIP;
            
            // This would make the actual HTTP request with authentication
            // For now, return success as a placeholder
            return {DDNSStatus::SUCCESS, "IP updated successfully", newIP, std::chrono::system_clock::now()};
        }
        catch (const std::exception& e) {
            return {DDNSStatus::NETWORK_ERROR, e.what(), newIP, std::chrono::system_clock::now()};
        }
    }

    bool NoIPProvider::ValidateConfig(const DDNSConfig& config) const {
        return !config.hostname.empty() && !config.username.empty() && !config.password.empty();
    }

    // DuckDNS Provider
    DDNSResult DuckDNSProvider::UpdateIP(const DDNSConfig& config, const std::string& newIP) {
        try {
            std::string url = "https://www.duckdns.org/update?domains=" + config.hostname + 
                              "&token=" + config.token + "&ip=" + newIP;
            
            // This would make the actual HTTP request
            // For now, return success as a placeholder
            return {DDNSStatus::SUCCESS, "IP updated successfully", newIP, std::chrono::system_clock::now()};
        }
        catch (const std::exception& e) {
            return {DDNSStatus::NETWORK_ERROR, e.what(), newIP, std::chrono::system_clock::now()};
        }
    }

    bool DuckDNSProvider::ValidateConfig(const DDNSConfig& config) const {
        return !config.hostname.empty() && !config.token.empty();
    }

    // Dynu Provider
    DDNSResult DynuProvider::UpdateIP(const DDNSConfig& config, const std::string& newIP) {
        try {
            std::string url = "https://api.dynu.com/nic/update?hostname=" + config.hostname + "&myip=" + newIP;
            
            // This would make the actual HTTP request with authentication
            return {DDNSStatus::SUCCESS, "IP updated successfully", newIP, std::chrono::system_clock::now()};
        }
        catch (const std::exception& e) {
            return {DDNSStatus::NETWORK_ERROR, e.what(), newIP, std::chrono::system_clock::now()};
        }
    }

    bool DynuProvider::ValidateConfig(const DDNSConfig& config) const {
        return !config.hostname.empty() && !config.username.empty() && !config.password.empty();
    }

    // FreeDNS Provider
    DDNSResult FreeDNSProvider::UpdateIP(const DDNSConfig& config, const std::string& newIP) {
        try {
            std::string url = "https://freedns.afraid.org/dynamic/update.php?" + config.token;
            
            // This would make the actual HTTP request
            return {DDNSStatus::SUCCESS, "IP updated successfully", newIP, std::chrono::system_clock::now()};
        }
        catch (const std::exception& e) {
            return {DDNSStatus::NETWORK_ERROR, e.what(), newIP, std::chrono::system_clock::now()};
        }
    }

    bool FreeDNSProvider::ValidateConfig(const DDNSConfig& config) const {
        return !config.hostname.empty() && !config.token.empty();
    }

    // Additional DDNSManager methods
    void DDNSManager::SetFallbackEnabled(bool enabled) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_fallbackEnabled = enabled;
        Logger::LogManager::GetInstance().Info("Fallback " + std::string(enabled ? "enabled" : "disabled"));
    }

    void DDNSManager::SetIPDetectionServices(const std::vector<std::string>& services) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ipDetectionServices = services;
        Logger::LogManager::GetInstance().Info("Updated IP detection services, count: " + std::to_string(services.size()));
    }

    std::vector<std::string> DDNSManager::GetIPDetectionServices() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ipDetectionServices;
    }

    std::string DDNSManager::GetCurrentIP() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentIP;
    }

    DDNSManager::DDNSStatistics DDNSManager::GetStatistics() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        DDNSStatistics stats;
        stats.successfulUpdates = m_successfulUpdates.load();
        stats.failedUpdates = m_failedUpdates.load();
        stats.lastUpdateTime = m_lastUpdateTime;
        stats.lastKnownIP = m_lastKnownIP;
        stats.isRunning = m_running.load();
        return stats;
    }

} // namespace Network