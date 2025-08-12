using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Threading;

namespace server.Models
{
    /// <summary>
    /// Model class representing the TCP server state and data
    /// </summary>
    public class ServerModel
    {
        private TcpListener? _listener;
        private UdpClient? _discoveryClient;
        private volatile bool _isServerRunning = false;
        private readonly List<TcpClient> _connectedClients = new();
        private readonly List<string> _logMessages = new();
        private readonly object _clientsLock = new();
        private readonly object _logLock = new();
        private const int MaxLogEntries = 5000;
        private CancellationTokenSource? _discoveryCancellationToken;

        /// <summary>
        /// Event raised when server status changes
        /// </summary>
        public event EventHandler<bool>? ServerStatusChanged;

        /// <summary>
        /// Event raised when a new log message is added
        /// </summary>
        public event EventHandler<string>? LogMessageAdded;

        /// <summary>
        /// Event raised when a client connects
        /// </summary>
        public event EventHandler<TcpClient>? ClientConnected;

        /// <summary>
        /// Event raised when a client disconnects
        /// </summary>
        public event EventHandler<TcpClient>? ClientDisconnected;

        /// <summary>
        /// Gets whether the server is currently running
        /// </summary>
        public bool IsServerRunning => _isServerRunning;

        /// <summary>
        /// Gets the current number of connected clients
        /// </summary>
        public int ConnectedClientsCount
        {
            get
            {
                lock (_clientsLock)
                {
                    return _connectedClients.Count;
                }
            }
        }

        /// <summary>
        /// Gets all log messages
        /// </summary>
        public IReadOnlyList<string> LogMessages
        {
            get
            {
                lock (_logLock)
                {
                    return new List<string>(_logMessages).AsReadOnly();
                }
            }
        }

        // Removed public exposure of the connected clients collection to limit surface area

        /// <summary>
        /// Sends an update command to all connected clients
        /// </summary>
        /// <param name="updateUrl">The URL where the new version can be downloaded</param>
        public void SendUpdateToAllClients(string updateUrl)
        {
            var updateCommand = $"UPDATE:{updateUrl}";
            var updateBytes = System.Text.Encoding.ASCII.GetBytes(updateCommand);
            
            TcpClient[] clientsSnapshot;
            lock (_clientsLock)
            {
                clientsSnapshot = _connectedClients.ToArray();
            }
            foreach (var client in clientsSnapshot)
            {
                try
                {
                    if (client.Connected)
                    {
                        client.GetStream().Write(updateBytes, 0, updateBytes.Length);
                        AddLogMessage($"Update command sent to client {((IPEndPoint)client.Client.RemoteEndPoint!).Address}");
                    }
                }
                catch (Exception ex)
                {
                    AddLogMessage($"ERROR: Failed to send update to client: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Sends an update command to a specific client
        /// </summary>
        /// <param name="client">The client to update</param>
        /// <param name="updateUrl">The URL where the new version can be downloaded</param>
        public void SendUpdateToClient(TcpClient client, string updateUrl)
        {
            try
            {
                if (client.Connected)
                {
                    var updateCommand = $"UPDATE:{updateUrl}";
                    var updateBytes = System.Text.Encoding.ASCII.GetBytes(updateCommand);
                    client.GetStream().Write(updateBytes, 0, updateBytes.Length);
                    
                    var clientAddress = ((IPEndPoint)client.Client.RemoteEndPoint!).Address;
                    AddLogMessage($"Update command sent to specific client {clientAddress}");
                }
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Failed to send update to specific client: {ex.Message}");
            }
        }

        /// <summary>
        /// Starts the TCP server on the specified port
        /// </summary>
        /// <param name="port">The port number to listen on</param>
        /// <returns>Task representing the async operation</returns>
        public async Task StartServerAsync(int port)
        {
            try
            {
                // Validate port number
                if (port < 1 || port > 65535)
                {
                    AddLogMessage($"ERROR: Invalid port number {port}. Please enter a number between 1-65535");
                    return;
                }

                // Create and start the listener
                _listener = new TcpListener(IPAddress.Any, port);
                _listener.Start();
                _isServerRunning = true;

                // Start discovery service
                StartDiscoveryService(port);

                // Update status and log
                OnServerStatusChanged(true);
                AddLogMessage($"Server started successfully on port {port}");
                AddLogMessage("Waiting for client connections...");

                // Start accepting clients
                await AcceptClientsAsync();
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Failed to start server: {ex.Message}");
                _isServerRunning = false;
                OnServerStatusChanged(false);
            }
        }

        /// <summary>
        /// Stops the TCP server and disconnects all clients
        /// </summary>
        public void StopServer()
        {
            try
            {
                _isServerRunning = false;

                // Stop discovery service
                StopDiscoveryService();

                // Stop the listener
                _listener?.Stop();

                // Disconnect all clients
                TcpClient[] clientsSnapshot;
                lock (_clientsLock)
                {
                    clientsSnapshot = _connectedClients.ToArray();
                }
                foreach (var client in clientsSnapshot)
                {
                    DisconnectClient(client);
                }

                AddLogMessage("Server stopped");
                OnServerStatusChanged(false);
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Failed to stop server: {ex.Message}");
            }
        }

        /// <summary>
        /// Accepts client connections asynchronously
        /// </summary>
        private async Task AcceptClientsAsync()
        {
            while (_isServerRunning && _listener != null)
            {
                try
                {
                    var client = await _listener.AcceptTcpClientAsync();
                    lock (_clientsLock)
                    {
                        _connectedClients.Add(client);
                    }
                    OnClientConnected(client);
                    
                    AddLogMessage($"Client connected from {((IPEndPoint)client.Client.RemoteEndPoint!).Address}:{((IPEndPoint)client.Client.RemoteEndPoint!).Port}");
                    
                    // Handle client in background
                    _ = Task.Run(() => HandleClientAsync(client));
                }
                catch (Exception) when (!_isServerRunning)
                {
                    // Expected when stopping server
                    break;
                }
                catch (Exception ex)
                {
                    AddLogMessage($"ERROR: Failed to accept client: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Handles communication with a connected client
        /// </summary>
        /// <param name="client">The TCP client to handle</param>
        private async Task HandleClientAsync(TcpClient client)
        {
            try
            {
                var stream = client.GetStream();
                var buffer = new byte[1024];

                while (_isServerRunning && client.Connected)
                {
                    var bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length).ConfigureAwait(false);
                    if (bytesRead == 0) break; // Client disconnected

                    var message = System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    AddLogMessage($"Received from client: {message}");

                    // Echo the message back to the client
                    var response = $"Server received: {message}";
                    var responseBytes = System.Text.Encoding.UTF8.GetBytes(response);
                    await stream.WriteAsync(responseBytes, 0, responseBytes.Length).ConfigureAwait(false);
                }
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Client communication error: {ex.Message}");
            }
            finally
            {
                DisconnectClient(client);
            }
        }

        /// <summary>
        /// Disconnects a specific client
        /// </summary>
        /// <param name="client">The client to disconnect</param>
        private void DisconnectClient(TcpClient client)
        {
            var removed = false;
            lock (_clientsLock)
            {
                removed = _connectedClients.Remove(client);
            }
            if (removed)
            {
                try
                {
                    client.Close();
                    OnClientDisconnected(client);
                    AddLogMessage("Client disconnected");
                }
                catch (Exception ex)
                {
                    AddLogMessage($"ERROR: Failed to disconnect client: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Adds a log message and raises the event
        /// </summary>
        /// <param name="message">The message to add</param>
        private void AddLogMessage(string message)
        {
            var timestamp = DateTime.Now.ToString("HH:mm:ss");
            var logEntry = $"[{timestamp}] {message}";
            lock (_logLock)
            {
                _logMessages.Add(logEntry);
                if (_logMessages.Count > MaxLogEntries)
                {
                    var removeCount = _logMessages.Count - MaxLogEntries;
                    _logMessages.RemoveRange(0, removeCount);
                }
            }
            OnLogMessageAdded(logEntry);
        }

        /// <summary>
        /// Clears all log messages
        /// </summary>
        public void ClearLogMessages()
        {
            lock (_logLock)
            {
                _logMessages.Clear();
            }
        }

        /// <summary>
        /// Gets all log messages as a single string
        /// </summary>
        /// <returns>All log messages concatenated with newlines</returns>
        public string GetLogMessagesAsString()
        {
            lock (_logLock)
            {
                return string.Join(Environment.NewLine, _logMessages);
            }
        }

        /// <summary>
        /// Raises the ServerStatusChanged event
        /// </summary>
        /// <param name="isRunning">Whether the server is running</param>
        protected virtual void OnServerStatusChanged(bool isRunning)
        {
            ServerStatusChanged?.Invoke(this, isRunning);
        }

        /// <summary>
        /// Raises the LogMessageAdded event
        /// </summary>
        /// <param name="message">The log message</param>
        protected virtual void OnLogMessageAdded(string message)
        {
            LogMessageAdded?.Invoke(this, message);
        }

        /// <summary>
        /// Raises the ClientConnected event
        /// </summary>
        /// <param name="client">The connected client</param>
        protected virtual void OnClientConnected(TcpClient client)
        {
            ClientConnected?.Invoke(this, client);
        }

        /// <summary>
        /// Raises the ClientDisconnected event
        /// </summary>
        /// <param name="client">The disconnected client</param>
        protected virtual void OnClientDisconnected(TcpClient client)
        {
            ClientDisconnected?.Invoke(this, client);
        }

        /// <summary>
        /// Starts the UDP discovery service
        /// </summary>
        /// <param name="port">The port number to advertise</param>
        private void StartDiscoveryService(int port)
        {
            try
            {
                _discoveryClient = new UdpClient();
                _discoveryClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                _discoveryClient.Client.Bind(new IPEndPoint(IPAddress.Any, port));

                _discoveryCancellationToken = new CancellationTokenSource();
                _ = Task.Run(() => DiscoveryListenerAsync(port), _discoveryCancellationToken.Token);

                AddLogMessage($"Discovery service started on port {port}");
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Failed to start discovery service: {ex.Message}");
            }
        }

        /// <summary>
        /// Stops the UDP discovery service
        /// </summary>
        private void StopDiscoveryService()
        {
            try
            {
                _discoveryCancellationToken?.Cancel();
                _discoveryClient?.Close();
                _discoveryClient?.Dispose();
                _discoveryClient = null;
                AddLogMessage("Discovery service stopped");
            }
            catch (Exception ex)
            {
                AddLogMessage($"ERROR: Failed to stop discovery service: {ex.Message}");
            }
        }

        /// <summary>
        /// Listens for discovery requests and responds with server information
        /// </summary>
        /// <param name="port">The port number to advertise</param>
        private async Task DiscoveryListenerAsync(int port)
        {
            try
            {
                while (!_discoveryCancellationToken?.Token.IsCancellationRequested == true)
                {
                    var result = await _discoveryClient!.ReceiveAsync().ConfigureAwait(false);
                    var message = System.Text.Encoding.ASCII.GetString(result.Buffer);

                    if (message.Contains("REMOTE_ACTIVITY_DISCOVERY"))
                    {
                        // Send response with server information
                        var response = "REMOTE_ACTIVITY_SERVER";
                        var responseBytes = System.Text.Encoding.ASCII.GetBytes(response);
                        
                        await _discoveryClient.SendAsync(responseBytes, responseBytes.Length, result.RemoteEndPoint).ConfigureAwait(false);
                        
                        AddLogMessage($"Discovery request from {result.RemoteEndPoint.Address}");
                    }
                }
            }
            catch (Exception ex)
            {
                if (!_discoveryCancellationToken?.Token.IsCancellationRequested == true)
                {
                    AddLogMessage($"ERROR: Discovery service error: {ex.Message}");
                }
            }
        }
    }
} 