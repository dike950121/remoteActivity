#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <chrono>
#include <thread>

namespace Logger {
    // Log levels
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        LOG_ERROR = 3,
        CRITICAL = 4
    };
    
    class LogManager {
    private:
        static std::unique_ptr<LogManager> instance;
        static std::mutex instanceMutex;
        
        std::ofstream logFile;
        std::mutex logMutex;
        LogLevel currentLevel;
        bool logToFile;
        bool logToConsole;
        std::string logDirectory;
        std::string logFileName;
        
        // Private constructor for singleton
        LogManager();
        
        // Helper methods
        std::string GetCurrentTimestamp() const;
        std::string LogLevelToString(LogLevel level) const;
        std::string FormatLogEntry(LogLevel level, const std::string& message, 
                                 const std::string& file = "", int line = 0) const;
        void WriteToFile(const std::string& entry);
        void WriteToConsole(const std::string& entry, LogLevel level);
        bool CreateLogDirectory();
        std::string GenerateLogFileName() const;
        
    public:
        // Singleton access
        static LogManager& GetInstance();
        
        // Initialization
        bool Initialize(const std::string& logDir = "logs", 
                       LogLevel level = LogLevel::INFO,
                       bool toFile = true, 
                       bool toConsole = false);
        
        // Configuration
        void SetLogLevel(LogLevel level);
        void SetLogToFile(bool enable);
        void SetLogToConsole(bool enable);
        void SetLogDirectory(const std::string& directory);
        
        // Logging methods
        void Log(LogLevel level, const std::string& message, 
                const std::string& file = "", int line = 0);
        
        void Debug(const std::string& message, const std::string& file = "", int line = 0);
        void Info(const std::string& message, const std::string& file = "", int line = 0);
        void Warning(const std::string& message, const std::string& file = "", int line = 0);
        void Error(const std::string& message, const std::string& file = "", int line = 0);
        void Critical(const std::string& message, const std::string& file = "", int line = 0);
        
        // Exception logging
        void LogException(const std::exception& ex, const std::string& context = "",
                         const std::string& file = "", int line = 0);
        
        // System information logging
        void LogSystemInfo();
        
        // Network logging
        void LogNetworkEvent(const std::string& event, const std::string& details = "");
        
        // Command logging
        void LogCommand(const std::string& command, const std::string& result = "", bool success = true);
        
        // File operation logging
        void LogFileOperation(const std::string& operation, const std::string& path, bool success = true);
        
        // Performance logging
        void LogPerformance(const std::string& operation, double durationMs);
        
        // Security logging
        void LogSecurityEvent(const std::string& event, const std::string& details = "");
        
        // Cleanup
        void Shutdown();
        
        // Destructor
        ~LogManager();
        
        // Delete copy constructor and assignment operator
        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;
    };
    
    // Performance timer class
    class PerformanceTimer {
    private:
        std::chrono::high_resolution_clock::time_point startTime;
        std::string operationName;
        bool autoLog;
        
    public:
        explicit PerformanceTimer(const std::string& operation, bool autoLogOnDestroy = true);
        ~PerformanceTimer();
        
        void Start();
        double Stop(); // Returns duration in milliseconds
        void Reset();
    };
    
    // Convenience macros for logging with file and line information
    #define LOG_DEBUG(msg) Logger::LogManager::GetInstance().Debug(msg, __FILE__, __LINE__)
    #define LOG_INFO(msg) Logger::LogManager::GetInstance().Info(msg, __FILE__, __LINE__)
    #define LOG_WARNING(msg) Logger::LogManager::GetInstance().Warning(msg, __FILE__, __LINE__)
    #define LOG_ERROR(msg) Logger::LogManager::GetInstance().Error(msg, __FILE__, __LINE__)
    #define LOG_CRITICAL(msg) Logger::LogManager::GetInstance().Critical(msg, __FILE__, __LINE__)
    
    // Exception logging macro
    #define LOG_EXCEPTION(ex, context) Logger::LogManager::GetInstance().LogException(ex, context, __FILE__, __LINE__)
    
    // Performance timing macro
    #define PERFORMANCE_TIMER(operation) Logger::PerformanceTimer timer(operation)
    
    // Network event logging
    #define LOG_NETWORK(event, details) Logger::LogManager::GetInstance().LogNetworkEvent(event, details)
    
    // Command logging
    #define LOG_COMMAND(cmd, result, success) Logger::LogManager::GetInstance().LogCommand(cmd, result, success)
    
    // File operation logging
    #define LOG_FILE_OP(op, path, success) Logger::LogManager::GetInstance().LogFileOperation(op, path, success)
    
    // Security event logging
    #define LOG_SECURITY(event, details) Logger::LogManager::GetInstance().LogSecurityEvent(event, details)
    
    // Utility functions
    std::string GetCurrentThreadId();
    std::string GetCurrentProcessIdString();
    std::string FormatBytes(size_t bytes);
    std::string FormatDuration(double milliseconds);
}