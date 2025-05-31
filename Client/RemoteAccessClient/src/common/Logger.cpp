#include "common/Logger.h"
#include "common/Config.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <ctime>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

namespace Logger {
    // Static member initialization
    std::unique_ptr<LogManager> LogManager::instance = nullptr;
    std::mutex LogManager::instanceMutex;
    
    LogManager::LogManager() 
        : currentLevel(LogLevel::INFO)
        , logToFile(true)
        , logToConsole(false)
        , logDirectory("logs")
    {
    }
    
    LogManager& LogManager::GetInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = std::unique_ptr<LogManager>(new LogManager());
        }
        return *instance;
    }
    
    bool LogManager::Initialize(const std::string& logDir, LogLevel level, bool toFile, bool toConsole) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        logDirectory = logDir;
        currentLevel = level;
        logToFile = toFile;
        logToConsole = toConsole;
        
        if (logToFile) {
            if (!CreateLogDirectory()) {
                return false;
            }
            
            logFileName = GenerateLogFileName();
            std::string fullPath = logDirectory + "/" + logFileName;
            
            logFile.open(fullPath, std::ios::app);
            if (!logFile.is_open()) {
                return false;
            }
            
            // Write initialization message
            std::string initMsg = "=== Logger initialized at " + GetCurrentTimestamp() + " ===";
            logFile << initMsg << std::endl;
            logFile.flush();
        }
        
        Info("Logger initialized successfully");
        return true;
    }
    
    void LogManager::SetLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        currentLevel = level;
    }
    
    void LogManager::SetLogToFile(bool enable) {
        std::lock_guard<std::mutex> lock(logMutex);
        logToFile = enable;
    }
    
    void LogManager::SetLogToConsole(bool enable) {
        std::lock_guard<std::mutex> lock(logMutex);
        logToConsole = enable;
    }
    
    void LogManager::SetLogDirectory(const std::string& directory) {
        std::lock_guard<std::mutex> lock(logMutex);
        logDirectory = directory;
    }
    
    void LogManager::Log(LogLevel level, const std::string& message, const std::string& file, int line) {
        if (level < currentLevel) {
            return;
        }
        
        std::string logEntry = FormatLogEntry(level, message, file, line);
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        if (logToFile) {
            WriteToFile(logEntry);
        }
        
        if (logToConsole) {
            WriteToConsole(logEntry, level);
        }
    }
    
    void LogManager::Debug(const std::string& message, const std::string& file, int line) {
        Log(LogLevel::DEBUG, message, file, line);
    }
    
    void LogManager::Info(const std::string& message, const std::string& file, int line) {
        Log(LogLevel::INFO, message, file, line);
    }
    
    void LogManager::Warning(const std::string& message, const std::string& file, int line) {
        Log(LogLevel::WARNING, message, file, line);
    }
    
    void LogManager::Error(const std::string& message, const std::string& file, int line) {
        Log(LogLevel::LOG_ERROR, message, file, line);
    }
    
    void LogManager::Critical(const std::string& message, const std::string& file, int line) {
        Log(LogLevel::CRITICAL, message, file, line);
    }
    
    void LogManager::LogException(const std::exception& ex, const std::string& context, const std::string& file, int line) {
        std::string message = "Exception: " + std::string(ex.what());
        if (!context.empty()) {
            message += " | Context: " + context;
        }
        Error(message, file, line);
    }
    
    void LogManager::LogSystemInfo() {
        Info("=== System Information ===");
        Info("Process ID: " + GetCurrentProcessIdString());
        Info("Thread ID: " + GetCurrentThreadId());
        
#ifdef _WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        Info("Processor Count: " + std::to_string(sysInfo.dwNumberOfProcessors));
        
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        Info("Total Physical Memory: " + FormatBytes(memInfo.ullTotalPhys));
        Info("Available Physical Memory: " + FormatBytes(memInfo.ullAvailPhys));
#endif
    }
    
    void LogManager::LogNetworkEvent(const std::string& event, const std::string& details) {
        std::string message = "[NETWORK] " + event;
        if (!details.empty()) {
            message += " | " + details;
        }
        Info(message);
    }
    
    void LogManager::LogCommand(const std::string& command, const std::string& result, bool success) {
        std::string message = "[COMMAND] " + command + " | Status: " + (success ? "SUCCESS" : "FAILED");
        if (!result.empty()) {
            message += " | Result: " + result;
        }
        if (success) {
            Info(message);
        } else {
            Error(message);
        }
    }
    
    void LogManager::LogFileOperation(const std::string& operation, const std::string& path, bool success) {
        std::string message = "[FILE] " + operation + " | Path: " + path + " | Status: " + (success ? "SUCCESS" : "FAILED");
        if (success) {
            Info(message);
        } else {
            Error(message);
        }
    }
    
    void LogManager::LogPerformance(const std::string& operation, double durationMs) {
        std::string message = "[PERFORMANCE] " + operation + " | Duration: " + FormatDuration(durationMs);
        Info(message);
    }
    
    void LogManager::LogSecurityEvent(const std::string& event, const std::string& details) {
        std::string message = "[SECURITY] " + event;
        if (!details.empty()) {
            message += " | " + details;
        }
        Warning(message);
    }
    
    void LogManager::Shutdown() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            Info("Logger shutting down");
            logFile << "=== Logger shutdown at " << GetCurrentTimestamp() << " ===" << std::endl;
            logFile.close();
        }
    }
    
    LogManager::~LogManager() {
        Shutdown();
    }
    
    std::string LogManager::GetCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string LogManager::LogLevelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::LOG_ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default: return "UNKNOWN";
        }
    }
    
    std::string LogManager::FormatLogEntry(LogLevel level, const std::string& message, const std::string& file, int line) const {
        std::stringstream ss;
        ss << "[" << GetCurrentTimestamp() << "] ";
        ss << "[" << LogLevelToString(level) << "] ";
        ss << "[" << GetCurrentThreadId() << "] ";
        
        if (!file.empty() && line > 0) {
            // Extract just the filename from the full path
            std::string filename = file;
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }
            ss << "[" << filename << ":" << line << "] ";
        }
        
        ss << message;
        return ss.str();
    }
    
    void LogManager::WriteToFile(const std::string& entry) {
        if (logFile.is_open()) {
            logFile << entry << std::endl;
            logFile.flush();
        }
    }
    
    void LogManager::WriteToConsole(const std::string& entry, LogLevel level) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // Default white
        
        switch (level) {
            case LogLevel::DEBUG:
                color = FOREGROUND_BLUE | FOREGROUND_GREEN; // Cyan
                break;
            case LogLevel::INFO:
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
                break;
            case LogLevel::WARNING:
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
                break;
            case LogLevel::LOG_ERROR:
                color = FOREGROUND_RED | FOREGROUND_INTENSITY; // Red
                break;
            case LogLevel::CRITICAL:
                color = FOREGROUND_RED | BACKGROUND_RED; // Red background
                break;
        }
        
        SetConsoleTextAttribute(hConsole, color);
        std::cout << entry << std::endl;
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        // ANSI color codes for Unix-like systems
        const char* colorCode = "\033[0m"; // Reset
        switch (level) {
            case LogLevel::DEBUG: colorCode = "\033[36m"; break; // Cyan
            case LogLevel::INFO: colorCode = "\033[37m"; break;  // White
            case LogLevel::WARNING: colorCode = "\033[33m"; break; // Yellow
            case LogLevel::LOG_ERROR: colorCode = "\033[31m"; break;   // Red
            case LogLevel::CRITICAL: colorCode = "\033[41m"; break; // Red background
        }
        std::cout << colorCode << entry << "\033[0m" << std::endl;
#endif
    }
    
    bool LogManager::CreateLogDirectory() {
        try {
            std::filesystem::create_directories(logDirectory);
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "Failed to create log directory: " << ex.what() << std::endl;
            return false;
        }
    }
    
    std::string LogManager::GenerateLogFileName() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "client_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".log";
        return ss.str();
    }
    
    // PerformanceTimer implementation
    PerformanceTimer::PerformanceTimer(const std::string& operation, bool autoLogOnDestroy)
        : operationName(operation), autoLog(autoLogOnDestroy) {
        Start();
    }
    
    PerformanceTimer::~PerformanceTimer() {
        if (autoLog) {
            double duration = Stop();
            LogManager::GetInstance().LogPerformance(operationName, duration);
        }
    }
    
    void PerformanceTimer::Start() {
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    double PerformanceTimer::Stop() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        return duration.count() / 1000.0; // Convert to milliseconds
    }
    
    void PerformanceTimer::Reset() {
        Start();
    }
    
    // Utility functions
    std::string GetCurrentThreadId() {
#ifdef _WIN32
        return std::to_string(static_cast<unsigned long>(::GetCurrentThreadId()));
#else
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
#endif
    }
    
    std::string GetCurrentProcessIdString() {
#ifdef _WIN32
        return std::to_string(static_cast<unsigned long>(::GetCurrentProcessId()));
#else
        return std::to_string(getpid());
#endif
    }
    
    std::string FormatBytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return ss.str();
    }
    
    std::string FormatDuration(double milliseconds) {
        if (milliseconds < 1000.0) {
            return std::to_string(static_cast<int>(milliseconds)) + "ms";
        } else if (milliseconds < 60000.0) {
            return std::to_string(static_cast<int>(milliseconds / 1000.0)) + "s";
        } else {
            int minutes = static_cast<int>(milliseconds / 60000.0);
            int seconds = static_cast<int>((milliseconds - minutes * 60000.0) / 1000.0);
            return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
        }
    }
}