# Modular TCP Server and Bot Client

WPF .NET server with modular C++ bot client for TCP communication.

## Quick Start

```bash
# Build everything
test_system_info.bat

# Run server
cd server && dotnet run

# Run bot (in new terminal)
cd bot && modular_bot.exe
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

- **WPF Server**: Modern UI with real-time logging and system information display
- **Modular C++ Bot**: Organized into specialized modules:
  - **SystemInfo**: System information collection
  - **NetworkClient**: TCP communication handling
  - **BotController**: Main bot logic and coordination
- **Port**: 5555 (configurable)
- **System Information**: Bot collects and sends detailed system information
- **Persistent Connection**: Bot maintains connection and auto-reconnects if disconnected
- **Periodic Updates**: Bot sends status updates every 30 seconds
- **Auto-reconnect**: 5-second delay between reconnection attempts

## Modular Architecture Benefits

- **Maintainability**: Each module has a single responsibility
- **Extensibility**: Easy to add new features or modules
- **Reusability**: Modules can be used independently
- **Debugging**: Easier to isolate and fix issues
- **Documentation**: Clear separation of concerns

## Requirements

- .NET 9.0 SDK
- MinGW-w64 (g++) or Visual Studio
- Windows 10/11 