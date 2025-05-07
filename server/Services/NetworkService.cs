using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using RemoteServer.Models;
using Serilog;

namespace RemoteServer.Services
{
    public class NetworkService : IDisposable
    {
        private const int Port = 8443;
        private TcpListener _listener;
        private volatile bool _isRunning;
        private CancellationTokenSource _cancellationTokenSource = new();
        private readonly ConcurrentDictionary<string, ConnectedClient> _clients = new();

        public event EventHandler<ConnectedClient>? ClientConnected;
        public event EventHandler<string>? ClientDisconnected;
        public event EventHandler<(string ClientId, Message Message)>? MessageReceived;

        public NetworkService(int port = 8443)
        {
            _listener = new TcpListener(IPAddress.Any, port);
            Log.Debug("NetworkService initialized with port {Port}", port);
        }

        public async Task StartAsync()
        {
            if (_isRunning) return;

            try
            {
                _listener.Start();
                _isRunning = true;
                Log.Information("Server started on port {Port}", ((IPEndPoint)_listener.LocalEndpoint).Port);

                while (!_cancellationTokenSource.Token.IsCancellationRequested)
                {
                    Log.Debug("Waiting for client connections...");
                    var client = await _listener.AcceptTcpClientAsync(_cancellationTokenSource.Token);
                    var ip = ((IPEndPoint)client.Client.RemoteEndPoint)?.Address.ToString() ?? "unknown";
                    Log.Information("New connection attempt from {IpAddress}", ip);
                    _ = HandleClientAsync(client);
                }
            }
            catch (OperationCanceledException)
            {
                Log.Information("Server shutdown requested");
                // Normal shutdown
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Critical error in network service");
            }
            finally
            {
                Log.Information("Server stopped");
            }
        }

        private async Task HandleClientAsync(TcpClient tcpClient)
        {
            var client = new ConnectedClient(tcpClient);
            if (_clients.TryAdd(client.Id, client))
            {
                try
                {
                    ClientConnected?.Invoke(this, client);
                    Log.Information("Client connected: {ClientId} from {IpAddress}", client.Id, client.IpAddress);

                    while (!_cancellationTokenSource.Token.IsCancellationRequested)
                    {
                        Log.Debug("Waiting for message from client {ClientId}", client.Id);
                        var message = await ReceiveMessageAsync(client);
                        if (message == null)
                        {
                            Log.Information("No message or connection closed for client {ClientId}", client.Id);
                            break;
                        }
                        
                        Log.Debug("Received message type {MessageType} with {DataLength} bytes from client {ClientId}", 
                            message.Type, message.DataLength, client.Id);
                        MessageReceived?.Invoke(this, (client.Id, message));
                    }
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "Error handling client {ClientId}", client.Id);
                }
                finally
                {
                    DisconnectClient(client.Id);
                }
            }
            else
            {
                Log.Warning("Failed to add client {ClientId} to client dictionary", client.Id);
                tcpClient.Close();
            }
        }

        private async Task<Message?> ReceiveMessageAsync(ConnectedClient client)
        {
            try
            {
                var typeBuffer = new byte[sizeof(int)];
                var lengthBuffer = new byte[sizeof(int)];

                Log.Debug("Reading message type from client {ClientId}", client.Id);
                if (!await ReadFullyAsync(client.Stream, typeBuffer))
                {
                    Log.Debug("Failed to read message type from client {ClientId}", client.Id);
                    return null;
                }

                Log.Debug("Reading message length from client {ClientId}", client.Id);
                if (!await ReadFullyAsync(client.Stream, lengthBuffer))
                {
                    Log.Debug("Failed to read message length from client {ClientId}", client.Id);
                    return null;
                }

                var type = (MessageType)BitConverter.ToInt32(typeBuffer);
                var length = BitConverter.ToInt32(lengthBuffer);

                Log.Debug("Message header parsed: Type={MessageType}, Length={Length} from client {ClientId}", 
                    type, length, client.Id);

                if (length > Message.MaxMessageSize)
                {
                    Log.Warning("Message too large from client {ClientId}: {Length} bytes", client.Id, length);
                    return null;
                }

                if (length < 0)
                {
                    Log.Warning("Invalid message length from client {ClientId}: {Length} bytes", client.Id, length);
                    return null;
                }

                var data = new byte[length];
                if (!await ReadFullyAsync(client.Stream, data))
                {
                    Log.Debug("Failed to read message data from client {ClientId}", client.Id);
                    return null;
                }

                Log.Debug("Successfully received complete message from client {ClientId}", client.Id);
                return new Message { Type = type, Data = data };
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error receiving message from client {ClientId}", client.Id);
                return null;
            }
        }

        public async Task SendMessageAsync(string clientId, Message message)
        {
            if (!_clients.TryGetValue(clientId, out var client))
            {
                Log.Warning("Attempted to send message to non-existent client {ClientId}", clientId);
                return;
            }

            try
            {
                Log.Debug("Sending message type {MessageType} with {DataLength} bytes to client {ClientId}", 
                    message.Type, message.DataLength, clientId);
                
                var typeBytes = BitConverter.GetBytes((int)message.Type);
                var lengthBytes = BitConverter.GetBytes(message.DataLength);

                await client.Stream.WriteAsync(typeBytes);
                await client.Stream.WriteAsync(lengthBytes);
                if (message.DataLength > 0)
                    await client.Stream.WriteAsync(message.Data);
                
                Log.Debug("Message sent successfully to client {ClientId}", clientId);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error sending message to client {ClientId}", clientId);
                DisconnectClient(clientId);
            }
        }

        private static async Task<bool> ReadFullyAsync(NetworkStream stream, byte[] buffer)
        {
            int bytesRead = 0;
            while (bytesRead < buffer.Length)
            {
                int read = await stream.ReadAsync(buffer.AsMemory(bytesRead, buffer.Length - bytesRead));
                if (read == 0) return false;
                bytesRead += read;
            }
            return true;
        }

        public void DisconnectClient(string clientId)
        {
            if (_clients.TryRemove(clientId, out var client))
            {
                try
                {
                    client.TcpClient.Close();
                    ClientDisconnected?.Invoke(this, clientId);
                    Log.Information("Client disconnected: {ClientId}", clientId);
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "Error disconnecting client {ClientId}", clientId);
                }
            }
            else
            {
                Log.Debug("Attempted to disconnect already disconnected client {ClientId}", clientId);
            }
        }

        public void Stop()
        {
            if (!_isRunning) return;

            Log.Information("Stopping network service...");
            _cancellationTokenSource.Cancel();
            
            int clientCount = _clients.Count;
            Log.Information("Closing {ClientCount} active connections", clientCount);
            
            foreach (var client in _clients.Values)
            {
                try
                {
                    client.TcpClient.Close();
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "Error closing client connection {ClientId}", client.Id);
                }
            }
            
            _clients.Clear();
            _listener.Stop();
            _isRunning = false;
            Log.Information("Network service stopped");
        }

        public void Dispose()
        {
            Log.Debug("Disposing NetworkService");
            Stop();
            _cancellationTokenSource.Dispose();
        }
    }
}