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
                    var client = await _listener.AcceptTcpClientAsync(_cancellationTokenSource.Token);
                    _ = HandleClientAsync(client);
                }
            }
            catch (OperationCanceledException)
            {
                // Normal shutdown
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error in network service");
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
                        var message = await ReceiveMessageAsync(client);
                        if (message == null) break;
                        
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
        }

        private async Task<Message?> ReceiveMessageAsync(ConnectedClient client)
        {
            try
            {
                var typeBuffer = new byte[sizeof(int)];
                var lengthBuffer = new byte[sizeof(int)];

                if (!await ReadFullyAsync(client.Stream, typeBuffer) ||
                    !await ReadFullyAsync(client.Stream, lengthBuffer))
                    return null;

                var type = (MessageType)BitConverter.ToInt32(typeBuffer);
                var length = BitConverter.ToInt32(lengthBuffer);

                if (length > Message.MaxMessageSize)
                {
                    Log.Warning("Message too large from client {ClientId}: {Length} bytes", client.Id, length);
                    return null;
                }

                var data = new byte[length];
                if (!await ReadFullyAsync(client.Stream, data))
                    return null;

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
                return;

            try
            {
                var typeBytes = BitConverter.GetBytes((int)message.Type);
                var lengthBytes = BitConverter.GetBytes(message.DataLength);

                await client.Stream.WriteAsync(typeBytes);
                await client.Stream.WriteAsync(lengthBytes);
                if (message.DataLength > 0)
                    await client.Stream.WriteAsync(message.Data);
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
        }

        public void Stop()
        {
            if (!_isRunning) return;

            _cancellationTokenSource.Cancel();
            foreach (var client in _clients.Values)
            {
                client.TcpClient.Close();
            }
            _clients.Clear();
            _listener.Stop();
            _isRunning = false;
        }

        public void Dispose()
        {
            Stop();
            _cancellationTokenSource.Dispose();
        }
    }
}