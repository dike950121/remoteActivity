# UDP Server Discovery

This bot uses UDP broadcast discovery to automatically find and connect to servers on the network.

## How UDP Discovery Works

### Discovery Process:
1. **Bot sends UDP broadcast**: `REMOTE_ACTIVITY_DISCOVERY`
2. **Server responds**: `REMOTE_ACTIVITY_SERVER`
3. **Bot connects**: To the discovered server automatically

### Requirements:
- Server must be running with discovery service enabled
- Both bot and server must be on the same network
- Firewall must allow UDP traffic on the server port (5555)
- Network must support UDP broadcasts

## Usage

### Starting the Bot:
```bash
modular_bot.exe
```

The bot will:
1. Send UDP discovery broadcasts
2. Listen for server responses
3. Connect to the first discovered server
4. Fall back to localhost (127.0.0.1) if no servers found

### Starting the Server:
```bash
dotnet run
```

The server automatically enables UDP discovery service when started.

## Network Requirements

### Firewall Settings:
- **UDP Port 5555**: Must be open for discovery
- **TCP Port 5555**: Must be open for bot connections
- **UDP Broadcast**: Must be allowed on the network

### Network Configuration:
- Both devices must be on the same subnet
- Network must support UDP broadcasts
- No network isolation between bot and server

## Troubleshooting

### Bot can't find server:
1. **Check server status**: Ensure server is running
2. **Verify network**: Both devices on same network
3. **Check firewall**: UDP port 5555 must be open
4. **Test connectivity**: Try ping between devices
5. **Check broadcast**: Network must support UDP broadcasts

### Discovery timeout:
1. **Increase timeout**: Server might be slow to respond
2. **Check server logs**: Verify discovery service is active
3. **Network issues**: Check for network isolation
4. **Firewall blocking**: Verify UDP traffic is allowed

### Connection fails after discovery:
1. **TCP port blocked**: Ensure TCP port 5555 is open
2. **Server overloaded**: Check server connection limits
3. **Network issues**: Verify TCP connectivity

## Building

```bash
# Windows
build.bat

# Or manually
g++ main.cpp system_info.cpp network_client.cpp bot_controller.cpp -o modular_bot.exe -lws2_32
```

## Debug Information

The bot provides detailed debug output:
- Discovery broadcast sent
- Server responses received
- Connection attempts
- Fallback to localhost

## Security Considerations

- UDP discovery is not encrypted
- Anyone on the network can see discovery packets
- Consider network security for production use
- Discovery packets are lightweight and safe
