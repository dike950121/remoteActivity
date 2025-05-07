#include "remote_client.h"

// Macro for unused parameters to avoid warnings
#define UNUSED(x) (void)(x)

static volatile BOOL running = TRUE;

BOOL WINAPI console_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        log_message(LOG_INFO, "Shutdown signal received, exiting...");
        running = FALSE;
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    // Mark parameters as unused
    UNUSED(argc);
    UNUSED(argv);

    // Initialize logging system
    log_init(NULL, LOG_DEBUG);
    log_message(LOG_INFO, "Remote client starting up");

    // Set up console handler for graceful shutdown
    SetConsoleCtrlHandler(console_handler, TRUE);
    log_message(LOG_DEBUG, "Console control handler registered");

    // Default configuration
    ClientConfig config = {
        .server_address = "127.0.0.1",
        .server_port = 8443,
        .log_file = NULL,      // We've already initialized the logger
        .log_level = LOG_DEBUG // Default log level
    };

    log_message(LOG_INFO, "Client configuration: server=%s, port=%d", 
                config.server_address, config.server_port);

    // Initialize the client
    log_message(LOG_DEBUG, "Initializing client...");
    RemoteClientError result = initialize_client(&config);
    if (result != RC_SUCCESS) {
        log_message(LOG_ERROR, "Failed to initialize client: WSA initialization error");
        log_close();
        return 1;
    }

    // Connect to server
    log_message(LOG_INFO, "Attempting to connect to server at %s:%d...", 
                config.server_address, config.server_port);
    result = connect_to_server();
    if (result != RC_SUCCESS) {
        log_message(LOG_ERROR, "Failed to connect to server at %s:%d", 
                    config.server_address, config.server_port);
        log_message(LOG_ERROR, "Make sure the server is running and the address/port are correct");
        cleanup_client();
        log_close();
        return 1;
    }

    log_message(LOG_INFO, "Connected to server at %s:%d", 
                config.server_address, config.server_port);

    // Main message processing loop
    Message message;
    while (running) {
        // Wait for and process messages
        log_message(LOG_DEBUG, "Waiting for server messages...");
        result = receive_message(&message);
        if (result != RC_SUCCESS) {
            if (running) {
                log_message(LOG_ERROR, "Error receiving message");
                break;
            }
            // If not running, this was a clean shutdown
            log_message(LOG_INFO, "Message loop terminated by shutdown signal");
            break;
        }

        log_message(LOG_DEBUG, "Received message type %d with %zu bytes", 
                    message.type, message.data_length);
        result = process_message(&message);
        if (result != RC_SUCCESS) {
            log_message(LOG_ERROR, "Error processing message");
            break;
        }
    }

    log_message(LOG_INFO, "Disconnecting from server...");
    disconnect_from_server();
    cleanup_client();
    log_message(LOG_INFO, "Client shutdown complete");
    log_close();
    return 0;
}