#include "remote_client.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <Windows.h>

static FILE* log_file = NULL;
static LogLevel minimum_log_level = LOG_INFO;
static CRITICAL_SECTION log_lock;
static int is_initialized = 0;

// Convert log level to string
static const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        default:          return "UNKNOWN";
    }
}

// Initialize the logger
void log_init(const char* log_path, LogLevel min_level) {
    if (is_initialized) {
        return;
    }

    // Create logs directory if it doesn't exist
    CreateDirectoryA("logs", NULL);

    // Initialize critical section for thread safety
    InitializeCriticalSection(&log_lock);
    
    minimum_log_level = min_level;
    
    char full_path[MAX_PATH];
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_s(&timeinfo, &now);
    
    if (log_path != NULL) {
        strncpy(full_path, log_path, MAX_PATH - 1);
    } else {
        // Create a default log file name with date stamp
        snprintf(full_path, MAX_PATH, "logs/client_log_%04d%02d%02d.txt",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    }
    
    // Open log file in append mode
    fopen_s(&log_file, full_path, "a");
    
    if (log_file != NULL) {
        is_initialized = 1;
        log_message(LOG_INFO, "Log system initialized");
    } else {
        fprintf(stderr, "ERROR: Could not open log file '%s'\n", full_path);
    }
}

// Close the logger
void log_close(void) {
    if (!is_initialized) {
        return;
    }
    
    EnterCriticalSection(&log_lock);
    
    if (log_file != NULL) {
        log_message(LOG_INFO, "Log system shutting down");
        fclose(log_file);
        log_file = NULL;
    }
    
    LeaveCriticalSection(&log_lock);
    DeleteCriticalSection(&log_lock);
    is_initialized = 0;
}

// Log a message with variable arguments
void log_message(LogLevel level, const char* format, ...) {
    if (level < minimum_log_level) {
        return; // Skip if below minimum level
    }
    
    if (!is_initialized) {
        // If log not initialized, print to stderr
        va_list args;
        va_start(args, format);
        fprintf(stderr, "[%s] ", log_level_to_string(level));
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    EnterCriticalSection(&log_lock);
    
    if (log_file != NULL) {
        time_t now;
        struct tm timeinfo;
        char time_str[26];
        
        // Get current time
        time(&now);
        localtime_s(&timeinfo, &now);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        // Print timestamp and log level
        fprintf(log_file, "%s [%s] ", time_str, log_level_to_string(level));
        
        // Print the message
        va_list args;
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        
        // Add newline and flush
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    LeaveCriticalSection(&log_lock);
} 