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
        return RC_ERROR_SYSTEM;
    }

    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return RC_ERROR_NETWORK;
    }

    // Store configuration
    current_config = *config;
    return RC_SUCCESS;
}

RemoteClientError connect_to_server(void) {
    struct sockaddr_in server_addr;
    
    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed with error: %d\n", WSAGetLastError());
        return RC_ERROR_NETWORK;
    }

    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(current_config.server_port);

    // Convert IP address string to binary
    if (parse_ipv4(current_config.server_address, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address format: %s\n", current_config.server_address);
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
        return RC_ERROR_NETWORK;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        switch(error) {
            case WSAECONNREFUSED:
                fprintf(stderr, "Connection refused - No server listening on the specified address/port\n");
                break;
            case WSAENETUNREACH:
                fprintf(stderr, "Network is unreachable\n");
                break;
            case WSAETIMEDOUT:
                fprintf(stderr, "Connection attempt timed out\n");
                break;
            default:
                fprintf(stderr, "Connection failed with error: %d\n", error);
        }
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
        return RC_ERROR_NETWORK;
    }

    return RC_SUCCESS;
}

RemoteClientError disconnect_from_server(void) {
    if (client_socket != INVALID_SOCKET) {
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    }
    return RC_SUCCESS;
}

RemoteClientError cleanup_client(void) {
    disconnect_from_server();
    WSACleanup();
    return RC_SUCCESS;
}

RemoteClientError receive_message(Message* message) {
    if (!message || client_socket == INVALID_SOCKET) {
        return RC_ERROR_SYSTEM;
    }

    // Use size_t for all size-related variables
    size_t total_received = 0;
    int received;

    // Receive message type
    received = recv(client_socket, (char*)&message->type, sizeof(MessageType), 0);
    if (received != sizeof(MessageType)) {
        return RC_ERROR_NETWORK;
    }

    // Receive data length
    received = recv(client_socket, (char*)&message->data_length, sizeof(size_t), 0);
    if (received != sizeof(size_t)) {
        return RC_ERROR_NETWORK;
    }

    if (message->data_length > MAX_MESSAGE_SIZE) {
        return RC_ERROR_SYSTEM;
    }

    // Receive message data
    while (total_received < message->data_length) {
        size_t remaining = message->data_length - total_received;
        received = recv(client_socket, 
                       message->data + total_received, 
                       (int)(remaining > INT_MAX ? INT_MAX : remaining),
                       0);
        if (received <= 0) {
            return RC_ERROR_NETWORK;
        }
        total_received += (size_t)received;
    }

    return RC_SUCCESS;
}

RemoteClientError send_message(const Message* message) {
    if (!message || client_socket == INVALID_SOCKET) {
        return RC_ERROR_SYSTEM;
    }

    // Use size_t for all size-related variables
    size_t total_sent = 0;
    int sent;

    // Send message type
    sent = send(client_socket, (const char*)&message->type, sizeof(MessageType), 0);
    if (sent != sizeof(MessageType)) {
        return RC_ERROR_NETWORK;
    }

    // Send data length
    sent = send(client_socket, (const char*)&message->data_length, sizeof(size_t), 0);
    if (sent != sizeof(size_t)) {
        return RC_ERROR_NETWORK;
    }

    // Send message data
    while (total_sent < message->data_length) {
        size_t remaining = message->data_length - total_sent;
        sent = send(client_socket, 
                   message->data + total_sent, 
                   (int)(remaining > INT_MAX ? INT_MAX : remaining),
                   0);
        if (sent <= 0) {
            return RC_ERROR_NETWORK;
        }
        total_sent += (size_t)sent;
    }

    return RC_SUCCESS;
}

RemoteClientError process_message(const Message* message) {
    if (!message) {
        return RC_ERROR_SYSTEM;
    }

    switch (message->type) {
        case MSG_TYPE_COMMAND:
            printf("Received command: %.*s\n", (int)message->data_length, message->data);
            break;
            
        case MSG_TYPE_RESPONSE:
            printf("Received response: %.*s\n", (int)message->data_length, message->data);
            break;
            
        case MSG_TYPE_ERROR:
            printf("Received error: %.*s\n", (int)message->data_length, message->data);
            break;
            
        default:
            printf("Received unknown message type\n");
            return RC_ERROR_SYSTEM;
    }

    return RC_SUCCESS;
}