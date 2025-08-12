using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using server.Models;
using server.Views;

namespace server.Controllers
{
    /// <summary>
    /// Controller class that coordinates between the Model and View
    /// </summary>
    public class ServerController
    {
        private readonly ServerModel _model;
        private readonly ServerView _view;
        private readonly Dispatcher _dispatcher;

        /// <summary>
        /// Initializes a new instance of the ServerController
        /// </summary>
        /// <param name="model">The server model</param>
        /// <param name="view">The server view</param>
        public ServerController(ServerModel model, ServerView view)
        {
            _model = model ?? throw new ArgumentNullException(nameof(model));
            _view = view ?? throw new ArgumentNullException(nameof(view));
            _dispatcher = Dispatcher.CurrentDispatcher;

            // Wire up model events to view updates
            _model.ServerStatusChanged += OnServerStatusChanged;
            _model.LogMessageAdded += OnLogMessageAdded;
            _model.ClientConnected += OnClientConnected;
            _model.ClientDisconnected += OnClientDisconnected;

            // Wire up view events to controller actions
            _view.StartStopRequested += OnStartStopRequested;
            _view.ClearLogRequested += OnClearLogRequested;
            _view.CopyLogRequested += OnCopyLogRequested;
            _view.UpdateAllRequested += OnUpdateAllRequested;

            // Initialize the view
            InitializeView();
        }

        /// <summary>
        /// Initializes the view with current model state
        /// </summary>
        private void InitializeView()
        {
            _view.UpdateServerStatus(_model.IsServerRunning);
            _view.UpdateConnectedClientsCount(_model.ConnectedClientsCount);
            _view.UpdateLogMessages(_model.LogMessages);
        }

        /// <summary>
        /// Handles start/stop server requests from the view
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="port">The port number to use</param>
        private async void OnStartStopRequested(object? sender, int port)
        {
            if (!_model.IsServerRunning)
            {
                await StartServer(port);
            }
            else
            {
                StopServer();
            }
        }

        /// <summary>
        /// Starts the server on the specified port
        /// </summary>
        /// <param name="port">The port number to listen on</param>
        private async Task StartServer(int port)
        {
            try
            {
                await _model.StartServerAsync(port);
            }
            catch (Exception ex)
            {
                await _dispatcher.InvokeAsync(() =>
                {
                    MessageBox.Show($"Failed to start server: {ex.Message}", "Error", 
                        MessageBoxButton.OK, MessageBoxImage.Error);
                });
            }
        }

        /// <summary>
        /// Stops the server
        /// </summary>
        private void StopServer()
        {
            try
            {
                _model.StopServer();
            }
            catch (Exception ex)
            {
                _dispatcher.Invoke(() =>
                {
                    MessageBox.Show($"Failed to stop server: {ex.Message}", "Error", 
                        MessageBoxButton.OK, MessageBoxImage.Error);
                });
            }
        }

        /// <summary>
        /// Handles clear log requests from the view
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void OnClearLogRequested(object? sender, EventArgs e)
        {
            _model.ClearLogMessages();
            _view.UpdateLogMessages(_model.LogMessages);
        }

        /// <summary>
        /// Handles copy log requests from the view
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void OnCopyLogRequested(object? sender, EventArgs e)
        {
            try
            {
                var logText = _model.GetLogMessagesAsString();
                if (!string.IsNullOrEmpty(logText))
                {
                    Clipboard.SetText(logText);
                    _view.ShowMessage("Log copied to clipboard", "Success");
                }
                else
                {
                    _view.ShowMessage("No log messages to copy", "Information");
                }
            }
            catch (Exception ex)
            {
                _view.ShowMessage($"Failed to copy log: {ex.Message}", "Error");
            }
        }

        /// <summary>
        /// Handles update all requests from the view
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void OnUpdateAllRequested(object? sender, EventArgs e)
        {
            try
            {
                // For now, we'll use a hardcoded update URL
                // In a real implementation, this would be configurable
                string updateUrl = "http://192.168.1.100:8080/updates/modular_bot.exe";
                SendUpdateToAllClients(updateUrl);
            }
            catch (Exception ex)
            {
                _view.ShowMessage($"Failed to send update command: {ex.Message}", "Error");
            }
        }

        /// <summary>
        /// Handles server status changes from the model
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="isRunning">Whether the server is running</param>
        private void OnServerStatusChanged(object? sender, bool isRunning)
        {
            _dispatcher.Invoke(() =>
            {
                _view.UpdateServerStatus(isRunning);
            });
        }

        /// <summary>
        /// Handles log message additions from the model
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="message">The log message</param>
        private void OnLogMessageAdded(object? sender, string message)
        {
            _dispatcher.Invoke(() =>
            {
                _view.AddLogMessage(message);
            });
        }

        /// <summary>
        /// Handles client connections from the model
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="client">The connected client</param>
        private void OnClientConnected(object? sender, System.Net.Sockets.TcpClient client)
        {
            _dispatcher.Invoke(() =>
            {
                _view.UpdateConnectedClientsCount(_model.ConnectedClientsCount);
            });
        }

        /// <summary>
        /// Handles client disconnections from the model
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="client">The disconnected client</param>
        private void OnClientDisconnected(object? sender, System.Net.Sockets.TcpClient client)
        {
            _dispatcher.Invoke(() =>
            {
                _view.UpdateConnectedClientsCount(_model.ConnectedClientsCount);
            });
        }

        /// <summary>
        /// Sends an update command to all connected clients
        /// </summary>
        /// <param name="updateUrl">The URL where the new version can be downloaded</param>
        public void SendUpdateToAllClients(string updateUrl)
        {
            try
            {
                _model.SendUpdateToAllClients(updateUrl);
                _view.ShowMessage($"Update command sent to {_model.ConnectedClientsCount} clients", "Success");
            }
            catch (Exception ex)
            {
                _view.ShowMessage($"Failed to send update: {ex.Message}", "Error");
            }
        }

        /// <summary>
        /// Sends an update command to a specific client
        /// </summary>
        /// <param name="client">The client to update</param>
        /// <param name="updateUrl">The URL where the new version can be downloaded</param>
        public void SendUpdateToClient(System.Net.Sockets.TcpClient client, string updateUrl)
        {
            try
            {
                _model.SendUpdateToClient(client, updateUrl);
                var clientAddress = ((System.Net.IPEndPoint)client.Client.RemoteEndPoint!).Address;
                _view.ShowMessage($"Update command sent to client {clientAddress}", "Success");
            }
            catch (Exception ex)
            {
                _view.ShowMessage($"Failed to send update to client: {ex.Message}", "Error");
            }
        }

        /// <summary>
        /// Disposes the controller and cleans up resources
        /// </summary>
        public void Dispose()
        {
            // Unwire events
            _model.ServerStatusChanged -= OnServerStatusChanged;
            _model.LogMessageAdded -= OnLogMessageAdded;
            _model.ClientConnected -= OnClientConnected;
            _model.ClientDisconnected -= OnClientDisconnected;

            _view.StartStopRequested -= OnStartStopRequested;
            _view.ClearLogRequested -= OnClearLogRequested;
            _view.CopyLogRequested -= OnCopyLogRequested;
            _view.UpdateAllRequested -= OnUpdateAllRequested;

            // Stop server if running
            if (_model.IsServerRunning)
            {
                _model.StopServer();
            }
        }
    }
} 