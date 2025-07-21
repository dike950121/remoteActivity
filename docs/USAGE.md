# Remote Activity Monitor - Usage Guide

## Overview

The Remote Activity Monitor System consists of two main components:
- **C++ Spy Bot Client**: Collects and transmits system information
- **WPF .NET Server**: Manages multiple clients and displays collected data

## System Requirements

### C++ Client (Spy Bot)
- **Windows**: Visual Studio 2019+ or MinGW-w64
- **Linux**: GCC 7+ with C++17 support
- **Dependencies**: CMake 3.10+, Socket libraries
- **Memory**: ~10MB RAM
- **Network**: TCP connectivity to server

### WPF Server
- **Windows**: .NET 8.0 SDK or Runtime
- **Memory**: ~50MB RAM + data storage
- **Network**: Inbound TCP port (default: 8888)

## Quick Start Guide

### Step 1: Build the Applications

#### Building C++ Client
```bash
# Windows
cd Client_SpyBot
build.bat

# Linux
cd Client_SpyBot
chmod +x build.sh
./build.sh
```

#### Building WPF Server
```bash
cd Server_WPF
build.bat  # Windows only
```

### Step 2: Configure the Client

Edit `Client_SpyBot/config.json`:
```json
{
    "server": {
        "ip": "127.0.0.1",    // Server IP address
        "port": 8888,         // Server port
        "auth_token": "your_token"
    },
    "collection": {
        "interval_seconds": 30,      // Data collection interval
        "collect_processes": true,   // Enable process monitoring
        "collect_network": true,     // Enable network monitoring
        "collect_performance": true  // Enable performance monitoring
    }
}
```

### Step 3: Start the Server

1. Launch the WPF Server application:
   ```bash
   cd Server_WPF/RemoteActivityServer
   dotnet run
   ```

2. Click **"Start Server"** in the application interface
3. The server will begin listening on the configured port (default: 8888)

### Step 4: Connect Clients

1. Start the C++ client on target machines:
   ```bash
   # Windows
   cd Client_SpyBot/build/bin
   SpyBot.exe

   # Linux
   cd Client_SpyBot/build/bin
   ./SpyBot
   ```

2. Clients will automatically connect to the server and begin transmitting data
3. View connected clients in the server interface

## Server Interface Guide

### Main Dashboard

#### Statistics Cards
- **Connected Clients**: Current number of active connections
- **Data Packets**: Total number of data packets received
- **Server Port**: The port the server is listening on

#### Client List
- View all connected clients with their details:
  - Display name (auto-generated or from client ID)
  - IP address
  - Connection timestamp
  - Connection status indicator

#### Command Panel
- **Command Input**: Enter commands to send to clients
- **Send to Selected**: Send command to the currently selected client
- **Broadcast All**: Send command to all connected clients
- **Disconnect Selected**: Forcefully disconnect a client

#### Server Log
- Real-time log of server activities
- Connection events, data reception, errors
- **Clear** button to reset the log

#### Client Details
- Detailed information about the selected client:
  - Client ID and connection information
  - Latest system data received
  - Formatted JSON view of collected data

### Server Controls

#### Start/Stop Server
- Toggle button in the header to start/stop the TCP server
- Automatically disconnects all clients when stopping
- Shows current server status

#### Data Export
- Export current client data to JSON file
- Includes server statistics and client information
- Files saved as `RemoteActivityExport_YYYYMMDD_HHMMSS.json`

## Client Data Collection

### System Information Collected

#### Operating System Info
- Platform (Windows/Linux)
- Version and build information
- Computer/hostname
- Current username

#### Hardware Information
- Total and available memory
- CPU information
- Processor count and architecture

#### Performance Metrics
- Memory usage percentage
- CPU idle time (Windows)
- Load averages (Linux)
- System uptime indicators

#### Process Information
- Running processes (limited to top 20)
- Process ID, name, memory usage
- Process paths where available

#### Network Connections
- Active TCP/UDP connections (limited to top 10)
- Local and remote addresses/ports
- Connection states
- Protocol information

### Data Transmission

- **Protocol**: TCP with length-prefixed JSON messages
- **Format**: Structured JSON with timestamps
- **Frequency**: Configurable interval (default: 30 seconds)
- **Heartbeat**: Automatic keepalive every 10 seconds
- **Compression**: None (for debugging visibility)

## Configuration Options

### Client Configuration (config.json)

```json
{
    "server": {
        "ip": "127.0.0.1",              // Target server IP
        "port": 8888,                   // Target server port
        "auth_token": "spy_bot_token"   // Authentication token
    },
    "collection": {
        "interval_seconds": 30,          // Collection interval
        "collect_processes": true,       // Process monitoring
        "collect_network": true,         // Network monitoring
        "collect_performance": true,     // Performance monitoring
        "collect_installed_software": false  // Software inventory
    },
    "network": {
        "connection_timeout_ms": 5000,   // Connection timeout
        "retry_attempts": 3,             // Connection retry count
        "heartbeat_interval_seconds": 10 // Heartbeat frequency
    },
    "logging": {
        "debug_enabled": true,           // Debug output
        "log_file": "spybot.log"        // Log file path
    },
    "client": {
        "client_name": "SpyBot_Windows", // Client identifier
        "version": "1.0.0"              // Client version
    }
}
```

### Server Configuration

The server configuration is primarily handled through the UI:
- **Port**: Default 8888, configurable in MainViewModel
- **Theme**: Material Design Dark theme with Blue/Cyan colors
- **Auto-export**: Not enabled by default
- **Logging**: Real-time in-memory logging with size limits

## Network Setup

### Firewall Configuration

#### Windows Server
```powershell
# Allow inbound connections on port 8888
New-NetFirewallRule -DisplayName "Remote Activity Server" -Direction Inbound -Protocol TCP -LocalPort 8888 -Action Allow
```

#### Linux Server
```bash
# UFW
sudo ufw allow 8888/tcp

# iptables
sudo iptables -I INPUT -p tcp --dport 8888 -j ACCEPT
```

### Port Forwarding

For internet access, configure port forwarding on your router:
1. Forward external port to internal server IP:8888
2. Use dynamic DNS if your external IP changes
3. Consider VPN for secure remote access

## Security Considerations

⚠️ **IMPORTANT SECURITY NOTICE** ⚠️

This application is designed for **educational and legitimate monitoring purposes only**. Please ensure:

1. **Legal Compliance**: Obtain proper authorization before monitoring any systems
2. **Network Security**: Run on isolated networks or VPNs when possible
3. **Authentication**: The current token-based auth is basic - implement proper security for production
4. **Data Encryption**: Communications are not encrypted - add TLS/SSL for sensitive environments
5. **Access Control**: Restrict server access to authorized users only
6. **Audit Trail**: Monitor who accesses the system and when

### Recommended Security Enhancements

- Implement SSL/TLS encryption for all communications
- Add user authentication and role-based access control
- Use certificate-based client authentication
- Add data encryption for stored information
- Implement audit logging and monitoring
- Add rate limiting and connection throttling
- Use secure token generation and rotation

## Troubleshooting

### Common Client Issues

#### Connection Refused
- Verify server IP and port in config.json
- Check if server is running and accessible
- Verify firewall settings on both client and server

#### Permission Denied
- Run client with appropriate privileges for system monitoring
- Some system information requires administrator/root access

#### Build Failures
- Ensure all dependencies are installed
- Verify CMake and compiler versions
- Check that all source files are present

### Common Server Issues

#### Port Already in Use
- Change server port in MainViewModel
- Kill existing processes using the port
- Check for other applications on the same port

#### Material Design Issues
- Ensure all NuGet packages are restored
- Verify .NET 8.0 is installed
- Check WPF dependencies

### Performance Optimization

#### Client Performance
- Reduce collection frequency for low-impact monitoring
- Disable unnecessary collection features
- Limit process/connection counts in SystemInfo.cpp

#### Server Performance
- Monitor memory usage with many clients
- Consider database storage for historical data
- Implement client connection limits if needed

## Extending the System

### Adding New Data Collection

1. Extend `SystemInfo` class with new collection methods
2. Update `DataCollector` to include new data types
3. Modify server models to handle new data structures
4. Update UI to display new information

### Custom Commands

1. Extend client command handling in `main.cpp`
2. Add command processing logic
3. Update server UI with new command options
4. Test command execution and error handling

### Data Storage

Consider adding persistent storage:
- SQLite for local database
- JSON files for simple storage
- External databases for enterprise use
- Time-series databases for historical analysis

## Support and Contributing

### Getting Help
- Check the troubleshooting section above
- Review application logs for error details
- Ensure all dependencies are properly installed
- Verify network connectivity and permissions

### Development Setup
- Follow coding standards for both C++ and C#
- Add comprehensive comments for new features
- Test on both Windows and Linux platforms
- Ensure proper error handling and logging

This system provides a foundation for legitimate system monitoring and can be extended based on specific requirements. 