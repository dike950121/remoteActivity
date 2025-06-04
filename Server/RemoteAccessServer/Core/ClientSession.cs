using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using RemoteAccessServer.Models;
using Newtonsoft.Json;

namespace RemoteAccessServer.Core
{
    /// <summary>
    /// Represents a session with a connected client
    /// </summary>
    public class ClientSession : IDisposable
    {
        private readonly TcpClient _tcpClient;
        private readonly NetworkStream _networkStream;
        private readonly SemaphoreSlim _sendSemaphore;
        private bool _isConnected;
        private DateTime _lastHeartbeat;

        public ClientInfo ClientInfo { get; }
        public bool IsConnected => _isConnected && _tcpClient?.Connected == true;

        // Events
        public event EventHandler<RemoteAccessServer.Models.ClientDisconnectedEventArgs>? Disconnected;
        public event EventHandler<RemoteAccessServer.Models.CommandExecutedEventArgs>? CommandExecuted;

        public ClientSession(TcpClient tcpClient, ClientInfo clientInfo)
        {
            _tcpClient = tcpClient ?? throw new ArgumentNullException(nameof(tcpClient));
            ClientInfo = clientInfo ?? throw new ArgumentNullException(nameof(clientInfo));
            _networkStream = _tcpClient.GetStream();
            _sendSemaphore = new SemaphoreSlim(1, 1);
            _isConnected = true;
            _lastHeartbeat = DateTime.Now;

            // Configure TCP client
            _tcpClient.ReceiveTimeout = 30000; // 30 seconds
            _tcpClient.SendTimeout = 30000;    // 30 seconds
        }

        /// <summary>
        /// Start the client session
        /// </summary>
        /// <param name="cancellationToken">Cancellation token</param>
        public async Task StartAsync(CancellationToken cancellationToken)
        {
            try
            {
                Logger.Log($"Starting session for client {ClientInfo.ClientId}");
                ClientInfo.SetOnline();

                // Send initial handshake
                await SendHandshakeAsync();

                // Start receiving messages
                await ReceiveMessagesAsync(cancellationToken);
            }
            catch (Exception ex)
            {
                Logger.LogError($"Error in client session {ClientInfo.ClientId}: {ex}");
            }
            finally
            {
                await DisconnectAsync();
            }
        }

        /// <summary>
        /// Send handshake message to client
        /// </summary>
        private async Task SendHandshakeAsync()
        {
            var handshake = new
            {
                Type = "handshake",
                ServerId = Environment.MachineName,
                Timestamp = DateTime.UtcNow,
                Version = "1.0.0"
            };

            await SendMessageAsync(handshake);
        }

        /// <summary>
        /// Receive messages from the client
        /// </summary>
        /// <param name="cancellationToken">Cancellation token</param>
        private async Task ReceiveMessagesAsync(CancellationToken cancellationToken)
        {
            var buffer = new byte[4096];
            var messageBuilder = new StringBuilder();

            while (_isConnected && !cancellationToken.IsCancellationRequested)
            {
                try
                {
                    var bytesRead = await _networkStream.ReadAsync(buffer, 0, buffer.Length, cancellationToken);
                    
                    if (bytesRead == 0)
                    {
                        Logger.Log($"Client {ClientInfo.ClientId} disconnected (0 bytes read)");
                        break;
                    }

                    var data = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    messageBuilder.Append(data);

                    // Process complete messages (assuming newline-delimited)
                    string messages = messageBuilder.ToString();
                    string[] lines = messages.Split('\n');
                    
                    // Keep the last incomplete line in the buffer
                    messageBuilder.Clear();
                    if (!messages.EndsWith("\n"))
                    {
                        messageBuilder.Append(lines[lines.Length - 1]);
                        Array.Resize(ref lines, lines.Length - 1);
                    }

                    // Process complete messages
                    foreach (var line in lines)
                    {
                        if (!string.IsNullOrWhiteSpace(line))
                        {
                            await ProcessMessageAsync(line.Trim());
                        }
                    }

                    ClientInfo.UpdateLastSeen();
                    ClientInfo.AddBytesReceived(bytesRead);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (IOException ex)
                {
                    Logger.LogWarning($"IO error with client {ClientInfo.ClientId}: {ex.Message}");
                    break;
                }
                catch (Exception ex)
                {
                    Logger.LogError($"Error receiving from client {ClientInfo.ClientId}: {ex}");
                    break;
                }
            }
        }

        /// <summary>
        /// Process a received message from the client
        /// </summary>
        /// <param name="message">The message to process</param>
        private async Task ProcessMessageAsync(string message)
        {
            try
            {
                var messageObj = JsonConvert.DeserializeObject<dynamic>(message);
                string? messageType = messageObj?.Type;

                switch (messageType)
                {
                    case "heartbeat":
                        await HandleHeartbeatAsync(messageObj);
                        break;
                    case "system_info":
                        await HandleSystemInfoAsync(messageObj);
                        break;
                    case "command_response":
                        await HandleCommandResponseAsync(messageObj);
                        break;
                    case "error":
                        await HandleErrorAsync(messageObj);
                        break;
                    default:
                        Logger.LogWarning($"Unknown message type from client {ClientInfo.ClientId}: {messageType}");
                        break;
                }
            }
            catch (Exception ex)
            {
                Logger.LogError($"Error processing message from client {ClientInfo.ClientId}: {ex}");
            }
        }

        /// <summary>
        /// Handle heartbeat message from client
        /// </summary>
        private async Task HandleHeartbeatAsync(dynamic message)
        {
            _lastHeartbeat = DateTime.Now;
            
            // Calculate ping
            if (DateTime.TryParse(message?.Timestamp?.ToString(), out DateTime clientTime))
            {
                var ping = (int)(DateTime.UtcNow - clientTime).TotalMilliseconds;
                ClientInfo.UpdatePing(Math.Max(0, ping));
            }
            
            // Send heartbeat response back to client
            var heartbeatResponse = new
            {
                Type = "heartbeat_response",
                Timestamp = DateTime.UtcNow,
                Sequence = message?.Sequence?.ToString() ?? "0"
            };
            
            await SendMessageAsync(heartbeatResponse);
        }

        /// <summary>
        /// Handle system info message from client
        /// </summary>
        private Task HandleSystemInfoAsync(dynamic message)
        {
            ClientInfo.OperatingSystem = message?.OS?.ToString() ?? string.Empty;
            ClientInfo.ComputerName = message?.ComputerName?.ToString() ?? string.Empty;
            ClientInfo.UserName = message?.UserName?.ToString() ?? string.Empty;
            ClientInfo.Version = message?.AgentVersion?.ToString() ?? string.Empty;
            return Task.CompletedTask;
        }

        /// <summary>
        /// Handle command response message from client
        /// </summary>
        private Task HandleCommandResponseAsync(dynamic message)
        {
            string commandId = message?.CommandId?.ToString() ?? string.Empty;
            string command = message?.Command?.ToString() ?? string.Empty;
            string response = message?.Response?.ToString() ?? string.Empty;

            ClientInfo.IncrementCommandsExecuted();
            CommandExecuted?.Invoke(this, new RemoteAccessServer.Models.CommandExecutedEventArgs(ClientInfo.ClientId, command, response));
            return Task.CompletedTask;
        }

        /// <summary>
        /// Handle error message from client
        /// </summary>
        private Task HandleErrorAsync(dynamic message)
        {
            Logger.LogError($"Client {ClientInfo.ClientId} reported error: {message?.ErrorMessage}");
            return Task.CompletedTask;
        }

        /// <summary>
        /// Send a heartbeat message to the client.
        /// </summary>
        public async Task SendHeartbeatAsync()
        {
            var heartbeat = new
            {
                Type = "heartbeat",
                Timestamp = DateTime.UtcNow
            };
            await SendMessageAsync(heartbeat);
            _lastHeartbeat = DateTime.Now;
        }

        /// <summary>
        /// Send a command to the client.
        /// </summary>
        /// <param name="command">The command to send.</param>
        public async Task SendCommandAsync(string command)
        {
            var commandMessage = new
            {
                Type = "command",
                Command = command
            };
            await SendMessageAsync(commandMessage);
        }

        /// <summary>
        /// Send a message to the client.
        /// </summary>
        /// <param name="message">The message object to send.</param>
        private async Task SendMessageAsync(object message)
        {
            await _sendSemaphore.WaitAsync();
            try
            {
                if (!_isConnected || !_tcpClient.Connected)
                {
                    Logger.LogWarning($"Attempted to send message to disconnected client {ClientInfo.ClientId}");
                    return;
                }

                var json = JsonConvert.SerializeObject(message);
                var data = Encoding.UTF8.GetBytes(json + "\n"); // Add newline as delimiter
                await _networkStream.WriteAsync(data, 0, data.Length);
                ClientInfo.AddBytesSent(data.Length);
            }
            catch (Exception ex)
            {
                Logger.LogError($"Error sending message to client {ClientInfo.ClientId}: {ex.Message}");
                await DisconnectAsync();
            }
            finally
            {
                _sendSemaphore.Release();
            }
        }

        /// <summary>
        /// Disconnect the client session.
        /// </summary>
        public async Task DisconnectAsync()
        {
            if (!_isConnected) return;

            _isConnected = false;
            Logger.Log($"Disconnecting client {ClientInfo.ClientId}");

            try
            {
                _tcpClient.Close();
                _networkStream.Dispose();
                _sendSemaphore.Dispose();
            }
            catch (Exception ex)
            {
                Logger.LogError($"Error during disconnect for client {ClientInfo.ClientId}: {ex.Message}");
            }
            finally
            {
                ClientInfo.SetOffline();
                Disconnected?.Invoke(this, new RemoteAccessServer.Models.ClientDisconnectedEventArgs(ClientInfo));
                await Task.CompletedTask;
            }
        }

        /// <summary>
        /// Dispose of managed and unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                _tcpClient?.Dispose();
                _networkStream?.Dispose();
                _sendSemaphore?.Dispose();
            }
        }
    }
}