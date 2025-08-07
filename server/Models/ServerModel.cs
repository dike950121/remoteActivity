using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace server.Models
{
    /// <summary>
    /// Model class representing the TCP server state and data
    /// </summary>
    public class ServerModel
    {
        private TcpListener? _listener;
        private bool _isServerRunning = false;
        private readonly List<TcpClient> _connectedClients = new();
        private readonly List<string> _logMessages = new();

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
        public int ConnectedClientsCount => _connectedClients.Count;

        /// <summary>
        /// Gets all log messages
        /// </summary>
        public IReadOnlyList<string> LogMessages => _logMessages.AsReadOnly();

        /// <summary>
        /// Gets all connected clients
        /// </summary>
        public IReadOnlyList<TcpClient> ConnectedClients => _connectedClients.AsReadOnly();

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
                
                // Stop the listener
                _listener?.Stop();
                _listener = null;

                // Disconnect all clients
                foreach (var client in _connectedClients.ToArray())
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
                    _connectedClients.Add(client);
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
                    var bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead == 0) break; // Client disconnected

                    var message = System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    AddLogMessage($"Received from client: {message}");

                    // Echo the message back to the client
                    var response = $"Server received: {message}";
                    var responseBytes = System.Text.Encoding.UTF8.GetBytes(response);
                    await stream.WriteAsync(responseBytes, 0, responseBytes.Length);
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
            if (_connectedClients.Remove(client))
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
            _logMessages.Add(logEntry);
            OnLogMessageAdded(logEntry);
        }

        /// <summary>
        /// Clears all log messages
        /// </summary>
        public void ClearLogMessages()
        {
            _logMessages.Clear();
        }

        /// <summary>
        /// Gets all log messages as a single string
        /// </summary>
        /// <returns>All log messages concatenated with newlines</returns>
        public string GetLogMessagesAsString()
        {
            return string.Join(Environment.NewLine, _logMessages);
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
    }
} 