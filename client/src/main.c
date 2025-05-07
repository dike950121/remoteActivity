#include "remote_client.h"

// Macro for unused parameters to avoid warnings
#define UNUSED(x) (void)(x)

static volatile BOOL running = TRUE;

BOOL WINAPI console_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        running = FALSE;
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    // Mark parameters as unused
    UNUSED(argc);
    UNUSED(argv);

    // Set up console handler for graceful shutdown
    SetConsoleCtrlHandler(console_handler, TRUE);

    // Default configuration
    ClientConfig config = {
        .server_address = "127.0.0.1",
        .server_port = 8443
    };

    // Initialize the client
    RemoteClientError result = initialize_client(&config);
    if (result != RC_SUCCESS) {
        fprintf(stderr, "Failed to initialize client: WSA initialization error\n");
        return 1;
    }

    // Connect to server
    printf("Attempting to connect to server at %s:%d...\n", config.server_address, config.server_port);
    result = connect_to_server();
    if (result != RC_SUCCESS) {
        fprintf(stderr, "Failed to connect to server at %s:%d\n", config.server_address, config.server_port);
        fprintf(stderr, "Make sure the server is running and the address/port are correct\n");
        cleanup_client();
        return 1;
    }

    printf("Connected to server at %s:%d\n", config.server_address, config.server_port);

    // Main message processing loop
    Message message;
    while (running) {
        // Wait for and process messages
        result = receive_message(&message);
        if (result != RC_SUCCESS) {
            if (running) {
                fprintf(stderr, "Error receiving message\n");
                break;
            }
            // If not running, this was a clean shutdown
            break;
        }

        result = process_message(&message);
        if (result != RC_SUCCESS) {
            fprintf(stderr, "Error processing message\n");
            break;
        }
    }

    printf("Disconnecting from server...\n");
    disconnect_from_server();
    cleanup_client();
    return 0;
}