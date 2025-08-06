# TCP Server and Bot Client

WPF .NET server with C++ bot client for TCP communication.

## Quick Start

```bash
# Build everything
test_complete.bat

# Run server
cd server && dotnet run

# Run bot (in new terminal)
cd bot && simple_bot.exe
```

## Build

### Server
```bash
cd server
dotnet build
dotnet run
```

### Bot
```bash
cd bot
build.bat
```

## Features

- **WPF Server**: Modern UI with real-time logging and persistent connections
- **C++ Bot**: Persistent TCP client with auto-reconnect
- **Port**: 5555 (configurable)
- **Persistent Connection**: Bot maintains connection and auto-reconnects if disconnected
- **Periodic Messages**: Bot sends messages every 10 seconds
- **Auto-reconnect**: 5-second delay between reconnection attempts

## Requirements

- .NET 9.0 SDK
- MinGW-w64 (g++) or Visual Studio
- Windows 10/11 