using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using RemoteAccessServer.Models;
using System.Linq;
using System.Collections.Generic;

namespace RemoteAccessServer.Core
{
    /// <summary>
    /// Manages the TCP server and client connections
    /// </summary>
    public class ServerManager : IDisposable
    {
        private TcpListener? _tcpListener;
        private CancellationTokenSource? _cancellationTokenSource;
        private readonly ConcurrentDictionary<string, ClientSession> _clients;
        private bool _isRunning;
        private int _port;

        public int Port => _port;
        public bool IsRunning => _isRunning;
        public int ClientCount => _clients.Count;

        // Events
        public event EventHandler<RemoteAccessServer.Models.ClientConnectedEventArgs>? ClientConnected;
        public event EventHandler<RemoteAccessServer.Models.ClientDisconnectedEventArgs>? ClientDisconnected;
        public event EventHandler? ServerStarted;
        public event EventHandler? ServerStopped;
        public event EventHandler<RemoteAccessServer.Models.CommandExecutedEventArgs>? CommandExecuted;

        public ServerManager()
        {
            _clients = new ConcurrentDictionary<string, ClientSession>();
        }

        /// <summary>
        /// Start the server on the specified port
        /// </summary>
        /// <param name="port">Port to listen on</param>
        public async Task StartAsync(int port)
        {
            if (_isRunning)
            {
                throw new InvalidOperationException("Server is already running");
            }

            try
            {
                _port = port;
                _cancellationTokenSource = new CancellationTokenSource();
                _tcpListener = new TcpListener(IPAddress.Any, port);
                _tcpListener.Start();
                _isRunning = true;

                Logger.Log($"Server started on port {port}");
                ServerStarted?.Invoke(this, EventArgs.Empty);

                // Start accepting clients
                _ = (Task)Task.Run(AcceptClientsAsync, _cancellationTokenSource!.Token);

                // Start heartbeat monitoring
                _ = (Task)Task.Run(MonitorClientsAsync, _cancellationTokenSource.Token);
            }
            catch (Exception ex)
            {
                Logger.LogError($"Failed to start server: {ex}");
                await StopAsync();
                throw;
            }
        }

        /// <summary>
        /// Stop the server
        /// </summary>
        public async Task StopAsync()
        {
            if (!_isRunning) return;

            try
            {
                _isRunning = false;
                _cancellationTokenSource?.Cancel();
                _tcpListener?.Stop();

                // Disconnect all clients
                var disconnectTasks = _clients.Values.Select(client => client.DisconnectAsync());
                await Task.WhenAll(disconnectTasks);
                _clients.Clear();

                Logger.Log("Server stopped");
                ServerStopped?.Invoke(this, EventArgs.Empty);
            }
            catch (Exception ex)
            {
                Logger.LogError($"Error stopping server: {ex}");
            }
            finally
            {
                _cancellationTokenSource?.Dispose();
                _cancellationTokenSource = null;
            }
        }

        /// <summary>
        /// Accept incoming client connections
        /// </summary>
        private async Task AcceptClientsAsync()
        {
            while (_isRunning && !_cancellationTokenSource!.Token.IsCancellationRequested)
            {
                try
                {
                    var tcpClient = await _tcpListener!.AcceptTcpClientAsync();
                    var clientEndpoint = tcpClient.Client.RemoteEndPoint as IPEndPoint;
                    var clientIp = clientEndpoint?.Address?.ToString() ?? "Unknown";

                    Logger.Log($"New client connection from {clientIp}");

                    // Create client session
                    var clientInfo = new ClientInfo(Guid.NewGuid().ToString("N")[..8].ToUpper(), clientIp);
                    var clientSession = new ClientSession(tcpClient, clientInfo);

                    // Subscribe to client events
                    clientSession.Disconnected += OnClientSessionDisconnected;
                    clientSession.CommandExecuted += OnClientCommandExecuted;

                    // Add to clients collection
                    _clients.TryAdd(clientInfo.ClientId, clientSession);

                    // Start client session
                    _ = Task.Run(() => clientSession.StartAsync(_cancellationTokenSource.Token));

                    // Notify UI
                    ClientConnected?.Invoke(this, new RemoteAccessServer.Models.ClientConnectedEventArgs(clientInfo));
                }
                catch (ObjectDisposedException)
                {
                    // Server is shutting down
                    break;
                }
                catch (Exception ex)
                {
                    Logger.LogError($"Error accepting client: {ex}");
                }
            }
        }

        /// <summary>
        /// Monitor client connections and send heartbeats
        /// </summary>
        private async Task MonitorClientsAsync()
        {
            while (_isRunning && !_cancellationTokenSource.Token.IsCancellationRequested)
            {
                try
                {
                    var clientsToRemove = new List<string>();

                    foreach (var kvp in _clients)
                    {
                        var clientId = kvp.Key;
                        var session = kvp.Value;

                        if (!session.IsConnected)
                        {
                            clientsToRemove.Add(clientId);
                            continue;
                        }

                        // Send heartbeat
                        try
                        {
                            await session.SendHeartbeatAsync();
                        }
                        catch (Exception ex)
                        {
                            Logger.LogWarning($"Failed to send heartbeat to {clientId}: {ex.Message}");
                            clientsToRemove.Add(clientId);
                        }
                    }

                    // Remove disconnected clients
                    foreach (var clientId in clientsToRemove)
                    {
                        if (_clients.TryRemove(clientId, out var session))
                        {
                            await session.DisconnectAsync();
                        }
                    }

                    await Task.Delay(30000, _cancellationTokenSource.Token); // 30 second intervals
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    Logger.LogError($"Error in client monitoring: {ex}");
                }
            }
        }

        /// <summary>
        /// Send a command to a specific client
        /// </summary>
        /// <param name="clientId">Target client ID</param>
        /// <param name="command">Command to send</param>
        /// <returns>Command response</returns>
        public async Task SendCommandAsync(string clientId, string command)
        {
            if (!_clients.TryGetValue(clientId, out var session))
            {
                throw new ArgumentException($"Client {clientId} not found");
            }

            try
            {
                await session.SendCommandAsync(command);
                CommandExecuted?.Invoke(this, new RemoteAccessServer.Models.CommandExecutedEventArgs(clientId, command, "Command sent"));
            }
            catch (Exception ex)
            {
                Logger.LogError($"Failed to send command to {clientId}: {ex}");
                throw;
            }
        }

        /// <summary>
        /// Get all connected clients
        /// </summary>
        /// <returns>List of client information</returns>
        public ClientInfo[] GetConnectedClients()
        {
            return _clients.Values.Select(s => s.ClientInfo).ToArray();
        }

        private void OnClientSessionDisconnected(object? sender, RemoteAccessServer.Models.ClientDisconnectedEventArgs e)
        {
            if (_clients.TryRemove(e.ClientId, out var session))
            {
                session.Dispose();
                ClientDisconnected?.Invoke(this, e);
            }
        }

        private void OnClientCommandExecuted(object? sender, RemoteAccessServer.Models.CommandExecutedEventArgs e)
        {
            CommandExecuted?.Invoke(this, e);
        }

        public void Dispose()
        {
            StopAsync().Wait(5000); // Wait up to 5 seconds for graceful shutdown
        }
    }

    // Event argument classes
    public class ClientConnectedEventArgs : EventArgs
    {
        public ClientInfo Client { get; }
        public ClientConnectedEventArgs(ClientInfo client) => Client = client;
    }

    public class ClientDisconnectedEventArgs : EventArgs
    {
        public string ClientId { get; }
        public ClientDisconnectedEventArgs(string clientId) => ClientId = clientId;
    }

    public class CommandExecutedEventArgs : EventArgs
    {
        public string? ClientId { get; }
        public string? Command { get; }
        public string? Response { get; }
        public CommandExecutedEventArgs(string clientId, string command, string response)
        {
            ClientId = clientId;
            Command = command;
            Response = response;
        }
    }
}