# Remote Activity Monitor System

A comprehensive remote monitoring system consisting of a C++ client spy bot and a WPF .NET server for managing multiple clients.

## Project Structure

```
remoteActivity/
├── Client_SpyBot/          # C++ Client Application
│   ├── src/                # Source files
│   ├── include/            # Header files
│   └── CMakeLists.txt      # Build configuration
├── Server_WPF/             # WPF .NET Server Application
│   ├── RemoteActivityServer/   # Main WPF project
│   └── RemoteActivityServer.sln # Visual Studio solution
└── docs/                   # Documentation
```

## Features

### C++ Client (Spy Bot)
- **System Information Collection**: Gathers OS, hardware, and process information
- **Network Communication**: TCP socket connection to server
- **JSON Data Serialization**: Structured data transfer
- **Cross-Platform**: Windows/Linux compatible
- **Lightweight**: Minimal resource footprint

### WPF .NET Server
- **Multi-Client Management**: Handle multiple spy bot connections
- **Real-time Dashboard**: Live data visualization
- **Client Control**: Send commands to connected bots
- **Data Logging**: Persistent storage of collected data
- **Modern UI**: WPF-based interface with Material Design

## Communication Protocol

- **Transport**: TCP Sockets
- **Data Format**: JSON
- **Port**: 8888 (configurable)
- **Authentication**: Basic token-based

## Quick Start

### Building the C++ Client
```bash
cd Client_SpyBot
mkdir build && cd build
cmake ..
make
./SpyBot
```

### Running the WPF Server
```bash
cd Server_WPF
dotnet build
dotnet run
```

## Requirements

### C++ Client
- C++17 or later
- CMake 3.10+
- JSON library (nlohmann/json)
- Socket libraries

### WPF Server
- .NET 8.0
- Visual Studio 2022 / VS Code
- Material Design WPF package

## Network Configuration

- Default Port: 8888
- Supports both local network and internet communication
- Configurable connection settings in both client and server

## Security Note

This project is for educational and legitimate monitoring purposes only. Ensure compliance with applicable laws and regulations when deploying. 