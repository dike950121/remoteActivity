# TCP Server - MVC Architecture

This project demonstrates the implementation of a TCP server using the Model-View-Controller (MVC) architectural pattern in WPF.

## Architecture Overview

### Model (ServerModel)
- **Location**: `Models/ServerModel.cs`
- **Responsibility**: Handles all business logic, data management, and server operations
- **Features**:
  - TCP server management (start/stop)
  - Client connection handling
  - Message processing and echoing
  - Log message management
  - Event-driven architecture for state changes

### View (ServerView)
- **Location**: `Views/ServerView.xaml` and `Views/ServerView.xaml.cs`
- **Responsibility**: User interface and user interaction handling
- **Features**:
  - Modern WPF interface with status indicators
  - Real-time log display with terminal-style appearance
  - User input validation
  - Event-driven communication with controller

### Controller (ServerController)
- **Location**: `Controllers/ServerController.cs`
- **Responsibility**: Coordinates between Model and View, handles application logic
- **Features**:
  - Mediates communication between Model and View
  - Handles user actions and translates them to model operations
  - Manages UI updates based on model state changes
  - Error handling and user feedback

## Key Benefits of MVC Architecture

1. **Separation of Concerns**: Each component has a single, well-defined responsibility
2. **Testability**: Components can be tested independently
3. **Maintainability**: Changes to one component don't affect others
4. **Reusability**: Components can be reused in different contexts
5. **Scalability**: Easy to extend with new features

## Project Structure

```
server/
├── Models/
│   └── ServerModel.cs          # Business logic and data management
├── Views/
│   ├── IServerView.cs          # View interface contract
│   ├── ServerView.xaml         # WPF UI definition
│   └── ServerView.xaml.cs      # View implementation
├── Controllers/
│   └── ServerController.cs     # Application logic and coordination
├── App.xaml                    # Application entry point
├── App.xaml.cs                 # MVC initialization
└── README_MVC.md              # This documentation
```

## Features

- **TCP Server**: Full-featured TCP server with configurable port
- **Client Management**: Real-time client connection monitoring
- **Logging**: Comprehensive logging with timestamps
- **Echo Server**: Responds to client messages
- **Modern UI**: Clean, professional WPF interface
- **Error Handling**: Robust error handling throughout the application

## Usage

1. Run the application
2. Enter a port number (default: 5555)
3. Click "Start Server" to begin listening for connections
4. Monitor client connections and server activity in the log
5. Use "Clear Log" to clear the display
6. Use "Copy Log" to copy log messages to clipboard

## Technical Implementation

### Event-Driven Communication
The MVC components communicate through events:
- Model raises events for state changes
- Controller listens to both Model and View events
- View updates based on controller instructions

### Thread Safety
- All UI updates are dispatched to the UI thread
- Background operations don't block the UI
- Proper resource cleanup on application exit

### Error Handling
- Comprehensive exception handling at all levels
- User-friendly error messages
- Graceful degradation on errors

## Building and Running

```bash
# Build the project
dotnet build

# Run the application
dotnet run
```

## Future Enhancements

- Configuration file support
- Multiple server instances
- Advanced client management
- Network statistics
- Plugin architecture
- Unit tests for each component 