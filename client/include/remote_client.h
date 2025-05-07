#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#define _WIN32_WINNT 0x0600  // Windows Vista or later, needed for inet_pton
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // For inet_pton
#include <time.h>      // For time functions in logging

// Define log levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// Error handling
typedef enum {
    RC_SUCCESS = 0,
    RC_ERROR_NETWORK,
    RC_ERROR_SYSTEM,
    RC_ERROR_MEMORY
} RemoteClientError;

// Client configuration
typedef struct {
    const char* server_address;
    unsigned short server_port;
    const char* log_file;      // Path to log file
    LogLevel log_level;        // Minimum log level to record
} ClientConfig;

// Function declarations
RemoteClientError initialize_client(const ClientConfig* config);
RemoteClientError cleanup_client(void);

// Logging functions
void log_message(LogLevel level, const char* format, ...);
void log_init(const char* log_file, LogLevel min_level);
void log_close(void);

// Network operations
RemoteClientError connect_to_server(void);
RemoteClientError disconnect_from_server(void);

// Message handling
#define MAX_MESSAGE_SIZE 4096

typedef enum {
    MSG_TYPE_COMMAND,
    MSG_TYPE_RESPONSE,
    MSG_TYPE_ERROR
} MessageType;

typedef struct {
    MessageType type;
    char data[MAX_MESSAGE_SIZE];
    size_t data_length;
} Message;

// Additional function declarations
RemoteClientError receive_message(Message* message);
RemoteClientError send_message(const Message* message);
RemoteClientError process_message(const Message* message);

// Screen capture function
RemoteClientError capture_and_send_screen(void);

#endif // REMOTE_CLIENT_H