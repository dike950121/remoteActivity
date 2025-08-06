using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace server;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    private TcpListener? _listener;
    private bool _isServerRunning = false;
    private readonly List<TcpClient> _connectedClients = new();
    private readonly Dispatcher _dispatcher;

    public MainWindow()
    {
        InitializeComponent();
        _dispatcher = Dispatcher.CurrentDispatcher;
        
        // Wire up event handlers
        StartStopButton.Click += StartStopButton_Click;
        ClearLogButton.Click += ClearLogButton_Click;
        CopyLogButton.Click += CopyLogButton_Click;
        
        // Log initial startup
        LogMessage("TCP Server initialized");
        LogMessage("Ready to start server on port 5555");
    }

    /// <summary>
    /// Handles the start/stop button click event
    /// </summary>
    private void StartStopButton_Click(object sender, RoutedEventArgs e)
    {
        if (!_isServerRunning)
        {
            _ = StartServer();
        }
        else
        {
            StopServer();
        }
    }

    /// <summary>
    /// Starts the TCP server on the specified port
    /// </summary>
    private Task StartServer()
    {
        try
        {
            // Validate port number
            if (!int.TryParse(PortTextBox.Text, out int port) || port < 1 || port > 65535)
            {
                LogMessage("ERROR: Invalid port number. Please enter a number between 1-65535");
                return Task.CompletedTask;
            }

            // Create and start the listener
            _listener = new TcpListener(IPAddress.Any, port);
            _listener.Start();
            _isServerRunning = true;

            // Update UI
            UpdateServerStatus(true);
            LogMessage($"Server started successfully on port {port}");
            LogMessage("Waiting for client connections...");

            // Start accepting clients in background
            _ = Task.Run(AcceptClientsAsync);
        }
        catch (Exception ex)
        {
            LogMessage($"ERROR: Failed to start server: {ex.Message}");
            _isServerRunning = false;
            UpdateServerStatus(false);
        }
        
        return Task.CompletedTask;
    }

    /// <summary>
    /// Stops the TCP server and disconnects all clients
    /// </summary>
    private void StopServer()
    {
        try
        {
            _isServerRunning = false;
            
            // Stop the listener
            _listener?.Stop();
            _listener = null;

            // Disconnect all clients
            lock (_connectedClients)
            {
                foreach (var client in _connectedClients)
                {
                    try
                    {
                        client.Close();
                    }
                    catch { /* Ignore errors when closing clients */ }
                }
                _connectedClients.Clear();
            }

            // Update UI
            UpdateServerStatus(false);
            LogMessage("Server stopped");
        }
        catch (Exception ex)
        {
            LogMessage($"ERROR: Failed to stop server: {ex.Message}");
        }
    }

    /// <summary>
    /// Continuously accepts new client connections
    /// </summary>
    private async Task AcceptClientsAsync()
    {
        while (_isServerRunning && _listener != null)
        {
            try
            {
                var client = await _listener.AcceptTcpClientAsync();
                var remoteEndPoint = client.Client.RemoteEndPoint as IPEndPoint;
                if (remoteEndPoint != null)
                {
                    LogMessage($"New client connected from {remoteEndPoint.Address}");
                }
                else
                {
                    LogMessage("New client connected from unknown address");
                }
                
                // Add client to list
                lock (_connectedClients)
                {
                    _connectedClients.Add(client);
                }

                // Handle client in background
                _ = Task.Run(() => HandleClientAsync(client));
            }
            catch (ObjectDisposedException)
            {
                // Listener was stopped, exit loop
                break;
            }
            catch (Exception ex)
            {
                LogMessage($"ERROR: Failed to accept client: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Handles communication with a connected client
    /// </summary>
    private async Task HandleClientAsync(TcpClient client)
    {
        var clientEndPoint = client.Client.RemoteEndPoint as IPEndPoint;
        var clientAddress = clientEndPoint?.Address.ToString() ?? "unknown";
        
        try
        {
            using var stream = client.GetStream();
            var buffer = new byte[1024];
            
            // Keep connection alive and handle multiple messages
            while (client.Connected && _isServerRunning)
            {
                try
                {
                    // Read client message
                    var bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                    if (bytesRead > 0)
                    {
                        var message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                        LogMessage($"Received from {clientAddress}: {message}");
                        
                        // Send response back to client
                        var response = $"Hello, Bot! Server received: {message}";
                        var responseBytes = Encoding.UTF8.GetBytes(response);
                        await stream.WriteAsync(responseBytes, 0, responseBytes.Length);
                        
                        LogMessage($"Sent response to {clientAddress}: {response}");
                    }
                    else if (bytesRead == 0)
                    {
                        // Client closed connection
                        LogMessage($"Client {clientAddress} closed connection");
                        break;
                    }
                }
                catch (IOException)
                {
                    // Client disconnected
                    LogMessage($"Client {clientAddress} disconnected");
                    break;
                }
                catch (Exception ex)
                {
                    LogMessage($"ERROR: Communication error with {clientAddress}: {ex.Message}");
                    break;
                }
            }
        }
        catch (Exception ex)
        {
            LogMessage($"ERROR: Connection error with {clientAddress}: {ex.Message}");
        }
        finally
        {
            // Remove client from list and close connection
            lock (_connectedClients)
            {
                _connectedClients.Remove(client);
            }
            
            try
            {
                client.Close();
                LogMessage($"Client {clientAddress} removed from server");
            }
            catch { /* Ignore errors when closing client */ }
        }
    }

    /// <summary>
    /// Updates the server status in the UI
    /// </summary>
    private void UpdateServerStatus(bool isRunning)
    {
        _dispatcher.Invoke(() =>
        {
            if (isRunning)
            {
                StartStopButton.Content = "Stop Server";
                StartStopButton.Background = System.Windows.Media.Brushes.OrangeRed;
                StatusText.Text = "Status: Running";
                StatusText.Foreground = System.Windows.Media.Brushes.Green;
                PortTextBox.IsEnabled = false;
            }
            else
            {
                StartStopButton.Content = "Start Server";
                StartStopButton.Background = System.Windows.Media.Brushes.ForestGreen;
                StatusText.Text = "Status: Stopped";
                StatusText.Foreground = System.Windows.Media.Brushes.Red;
                PortTextBox.IsEnabled = true;
            }
        });
    }

    /// <summary>
    /// Logs a message to the UI with timestamp
    /// </summary>
    private void LogMessage(string message)
    {
        _dispatcher.Invoke(() =>
        {
            var timestamp = DateTime.Now.ToString("HH:mm:ss");
            LogTextBox.AppendText($"[{timestamp}] {message}\n");
            LogTextBox.ScrollToEnd();
        });
    }

    /// <summary>
    /// Handles the clear log button click event
    /// </summary>
    private void ClearLogButton_Click(object sender, RoutedEventArgs e)
    {
        LogTextBox.Clear();
        LogMessage("Log cleared");
    }

    /// <summary>
    /// Handles the copy log button click event
    /// </summary>
    private void CopyLogButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            Clipboard.SetText(LogTextBox.Text);
            LogMessage("Log copied to clipboard");
        }
        catch (Exception ex)
        {
            LogMessage($"ERROR: Failed to copy log: {ex.Message}");
        }
    }

    /// <summary>
    /// Cleanup when window is closing
    /// </summary>
    protected override void OnClosed(EventArgs e)
    {
        StopServer();
        base.OnClosed(e);
    }
}