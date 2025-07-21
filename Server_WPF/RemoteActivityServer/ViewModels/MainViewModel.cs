using System.Collections.ObjectModel;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RemoteActivityServer.Models;
using RemoteActivityServer.Services;

namespace RemoteActivityServer.ViewModels
{
    /// <summary>
    /// Main view model for the Remote Activity Server application
    /// </summary>
    public partial class MainViewModel : ObservableObject
    {
        private readonly TcpServerService _tcpServerService;
        
        [ObservableProperty]
        private bool _isServerRunning;
        
        [ObservableProperty]
        private string _serverStatus = "Server Stopped";
        
        [ObservableProperty]
        private int _serverPort = 8888;
        
        [ObservableProperty]
        private ClientConnection? _selectedClient;
        
        [ObservableProperty]
        private string _logOutput = string.Empty;
        
        [ObservableProperty]
        private string _commandInput = string.Empty;
        
        [ObservableProperty]
        private int _totalClientsConnected;
        
        [ObservableProperty]
        private int _totalDataPacketsReceived;
        
        [ObservableProperty]
        private DateTime _serverStartTime = DateTime.Now;
        
        /// <summary>
        /// Observable collection of connected clients
        /// </summary>
        public ObservableCollection<ClientConnection> ConnectedClients { get; private set; }
        
        /// <summary>
        /// Command to start/stop the server
        /// </summary>
        public ICommand ToggleServerCommand { get; }
        
        /// <summary>
        /// Command to send command to selected client
        /// </summary>
        public ICommand SendCommandCommand { get; }
        
        /// <summary>
        /// Command to broadcast command to all clients
        /// </summary>
        public ICommand BroadcastCommandCommand { get; }
        
        /// <summary>
        /// Command to disconnect selected client
        /// </summary>
        public ICommand DisconnectClientCommand { get; }
        
        /// <summary>
        /// Command to clear log output
        /// </summary>
        public ICommand ClearLogCommand { get; }
        
        /// <summary>
        /// Command to export client data
        /// </summary>
        public ICommand ExportDataCommand { get; }
        
        /// <summary>
        /// Constructor for MainViewModel
        /// </summary>
        public MainViewModel()
        {
            _tcpServerService = new TcpServerService(ServerPort);
            ConnectedClients = _tcpServerService.ConnectedClients;
            
            // Initialize commands
            ToggleServerCommand = new AsyncRelayCommand(ToggleServerAsync);
            SendCommandCommand = new AsyncRelayCommand(SendCommandAsync, () => SelectedClient != null && !string.IsNullOrWhiteSpace(CommandInput));
            BroadcastCommandCommand = new AsyncRelayCommand(BroadcastCommandAsync, () => !string.IsNullOrWhiteSpace(CommandInput));
            DisconnectClientCommand = new AsyncRelayCommand(DisconnectClientAsync, () => SelectedClient != null);
            ClearLogCommand = new RelayCommand(ClearLog);
            ExportDataCommand = new AsyncRelayCommand(ExportDataAsync);
            
            // Subscribe to server events
            _tcpServerService.ClientConnected += OnClientConnected;
            _tcpServerService.ClientDisconnected += OnClientDisconnected;
            _tcpServerService.DataReceived += OnDataReceived;
            _tcpServerService.StatusChanged += OnStatusChanged;
            
            // Update command can execute state
            PropertyChanged += (s, e) =>
            {
                if (e.PropertyName == nameof(SelectedClient) || e.PropertyName == nameof(CommandInput))
                {
                    ((AsyncRelayCommand)SendCommandCommand).NotifyCanExecuteChanged();
                    ((AsyncRelayCommand)BroadcastCommandCommand).NotifyCanExecuteChanged();
                    ((AsyncRelayCommand)DisconnectClientCommand).NotifyCanExecuteChanged();
                }
            };
            
            AddLogEntry("Remote Activity Server initialized");
        }
        
        /// <summary>
        /// Toggle server start/stop
        /// </summary>
        private async Task ToggleServerAsync()
        {
            try
            {
                if (IsServerRunning)
                {
                    await _tcpServerService.StopServerAsync();
                    IsServerRunning = false;
                    ServerStatus = "Server Stopped";
                    AddLogEntry("Server stopped by user");
                }
                else
                {
                    var started = await _tcpServerService.StartServerAsync();
                    if (started)
                    {
                        IsServerRunning = true;
                        ServerStatus = $"Server Running on Port {ServerPort}";
                        ServerStartTime = DateTime.Now;
                        TotalClientsConnected = 0;
                        TotalDataPacketsReceived = 0;
                        AddLogEntry($"Server started successfully on port {ServerPort}");
                    }
                    else
                    {
                        AddLogEntry("Failed to start server");
                    }
                }
            }
            catch (Exception ex)
            {
                AddLogEntry($"Error toggling server: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Send command to selected client
        /// </summary>
        private async Task SendCommandAsync()
        {
            if (SelectedClient == null || string.IsNullOrWhiteSpace(CommandInput)) return;
            
            try
            {
                var success = await _tcpServerService.SendCommandToClientAsync(SelectedClient, CommandInput);
                if (success)
                {
                    AddLogEntry($"Command sent to {SelectedClient.DisplayName}: {CommandInput}");
                    CommandInput = string.Empty;
                }
                else
                {
                    AddLogEntry($"Failed to send command to {SelectedClient.DisplayName}");
                }
            }
            catch (Exception ex)
            {
                AddLogEntry($"Error sending command: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Broadcast command to all connected clients
        /// </summary>
        private async Task BroadcastCommandAsync()
        {
            if (string.IsNullOrWhiteSpace(CommandInput)) return;
            
            try
            {
                var successCount = await _tcpServerService.BroadcastCommandAsync(CommandInput);
                AddLogEntry($"Command broadcasted to {successCount} clients: {CommandInput}");
                CommandInput = string.Empty;
            }
            catch (Exception ex)
            {
                AddLogEntry($"Error broadcasting command: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Disconnect selected client
        /// </summary>
        private async Task DisconnectClientAsync()
        {
            if (SelectedClient == null) return;
            
            try
            {
                await _tcpServerService.DisconnectClientAsync(SelectedClient);
                AddLogEntry($"Client disconnected: {SelectedClient.DisplayName}");
                SelectedClient = null;
            }
            catch (Exception ex)
            {
                AddLogEntry($"Error disconnecting client: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Clear log output
        /// </summary>
        private void ClearLog()
        {
            LogOutput = string.Empty;
        }
        
        /// <summary>
        /// Export client data to file
        /// </summary>
        private async Task ExportDataAsync()
        {
            try
            {
                // Simple export to JSON file
                var exportData = new
                {
                    ExportTimestamp = DateTime.Now,
                    ServerInfo = new
                    {
                        Port = ServerPort,
                        IsRunning = IsServerRunning,
                        StartTime = ServerStartTime,
                        TotalClients = TotalClientsConnected
                    },
                    Clients = ConnectedClients.Select(c => new
                    {
                        c.ClientId,
                        c.DisplayName,
                        c.IpAddress,
                        c.ConnectedAt,
                        c.LastDataReceived,
                        c.Status,
                        LatestDataTimestamp = c.LatestData?.GetTimestamp()
                    }).ToArray()
                };
                
                var json = System.Text.Json.JsonSerializer.Serialize(exportData, new System.Text.Json.JsonSerializerOptions { WriteIndented = true });
                var fileName = $"RemoteActivityExport_{DateTime.Now:yyyyMMdd_HHmmss}.json";
                await File.WriteAllTextAsync(fileName, json);
                
                AddLogEntry($"Data exported to {fileName}");
            }
            catch (Exception ex)
            {
                AddLogEntry($"Error exporting data: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Handle client connected event
        /// </summary>
        private void OnClientConnected(object? sender, ClientConnection client)
        {
            TotalClientsConnected++;
            AddLogEntry($"New client connected: {client.DisplayName} from {client.IpAddress}");
        }
        
        /// <summary>
        /// Handle client disconnected event
        /// </summary>
        private void OnClientDisconnected(object? sender, ClientConnection client)
        {
            AddLogEntry($"Client disconnected: {client.DisplayName}");
            
            // Clear selection if this was the selected client
            if (SelectedClient == client)
            {
                SelectedClient = null;
            }
        }
        
        /// <summary>
        /// Handle data received from client
        /// </summary>
        private void OnDataReceived(object? sender, (ClientConnection Client, SystemData Data) eventArgs)
        {
            TotalDataPacketsReceived++;
            AddLogEntry($"Data received from {eventArgs.Client.DisplayName} - Type: {eventArgs.Data.MessageType}");
        }
        
        /// <summary>
        /// Handle server status changes
        /// </summary>
        private void OnStatusChanged(object? sender, string status)
        {
            AddLogEntry($"Server: {status}");
        }
        
        /// <summary>
        /// Add entry to log output
        /// </summary>
        /// <param name="message">Log message</param>
        private void AddLogEntry(string message)
        {
            var timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
            var logEntry = $"[{timestamp}] {message}\n";
            
            // Update on UI thread
            Application.Current.Dispatcher.Invoke(() =>
            {
                LogOutput += logEntry;
                
                // Keep log size manageable (last 1000 lines)
                var lines = LogOutput.Split('\n');
                if (lines.Length > 1000)
                {
                    LogOutput = string.Join('\n', lines.Skip(lines.Length - 1000));
                }
            });
        }
        
        /// <summary>
        /// Get uptime string
        /// </summary>
        public string UptimeString
        {
            get
            {
                if (!IsServerRunning) return "00:00:00";
                var uptime = DateTime.Now - ServerStartTime;
                return $"{uptime.Hours:D2}:{uptime.Minutes:D2}:{uptime.Seconds:D2}";
            }
        }
    }
} 