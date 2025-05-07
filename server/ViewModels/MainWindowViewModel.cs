using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RemoteServer.Models;
using RemoteServer.Services;
using System.Text;
using Serilog;

namespace RemoteServer.ViewModels
{
    public partial class MainWindowViewModel : ObservableObject
    {
        private readonly NetworkService _networkService;
        private ScreenSharingWindow? _screenSharingWindow;
        
        [ObservableProperty]
        private bool _isServerRunning;
        
        [ObservableProperty]
        private string _serverStatus = "Server Stopped";

        [ObservableProperty]
        private string? _selectedClientId;

        public ObservableCollection<ConnectedClient> ConnectedClients { get; } = new();

        public ObservableCollection<string> Commands { get; } = new ObservableCollection<string>
        {
            "screen sharing"
        };

        [ObservableProperty]
        private string? _selectedCommand;

        public MainWindowViewModel(NetworkService networkService)
        {
            _networkService = networkService;
            _networkService.ClientConnected += OnClientConnected;
            _networkService.ClientDisconnected += OnClientDisconnected;
            _networkService.MessageReceived += OnMessageReceived;
        }

        [RelayCommand]
        private async Task ToggleServerAsync()
        {
            if (!IsServerRunning)
            {
                IsServerRunning = true;
                ServerStatus = "Server Running...";
                await _networkService.StartAsync();
            }
            else
            {
                IsServerRunning = false;
                ServerStatus = "Server Stopped";
                _networkService.Stop();
                ConnectedClients.Clear();
            }
        }

        [RelayCommand]
        private async Task SendCommandAsync()
        {
            if (string.IsNullOrEmpty(SelectedClientId) || string.IsNullOrEmpty(SelectedCommand))
                return;

            if (SelectedCommand == "screen sharing")
            {
                // Open the screen sharing dialog
                App.Current.Dispatcher.Invoke(() =>
                {
                    _screenSharingWindow = new ScreenSharingWindow();
                    _screenSharingWindow.Closed += (s, e) => _screenSharingWindow = null;
                    _screenSharingWindow.Show();
                });
            }

            var message = new Message
            {
                Type = MessageType.Command,
                Data = Encoding.UTF8.GetBytes(SelectedCommand)
            };

            await _networkService.SendMessageAsync(SelectedClientId, message);
        }

        [RelayCommand]
        private void DisconnectClient()
        {
            if (string.IsNullOrEmpty(SelectedClientId))
                return;

            _networkService.DisconnectClient(SelectedClientId);
        }

        private void OnClientConnected(object? sender, ConnectedClient client)
        {
            App.Current.Dispatcher.Invoke(() =>
            {
                ConnectedClients.Add(client);
                ServerStatus = $"Client connected: {client.IpAddress}";
            });
        }

        private void OnClientDisconnected(object? sender, string clientId)
        {
            App.Current.Dispatcher.Invoke(() =>
            {
                var client = ConnectedClients.FirstOrDefault(c => c.Id == clientId);
                if (client != null)
                {
                    ConnectedClients.Remove(client);
                    ServerStatus = $"Client disconnected: {client.IpAddress}";
                }
            });
        }

        private void OnMessageReceived(object? sender, (string ClientId, Message Message) args)
        {
            try
            {
                // Check if this is image data
                if (_screenSharingWindow != null)
                {
                    // Try to parse as text first to check for header
                    string headerText = Encoding.UTF8.GetString(args.Message.Data, 0, Math.Min(50, args.Message.Data.Length));
                    
                    // Check if it's an image header
                    if (headerText.StartsWith("SCREEN_IMAGE:"))
                    {
                        // It's an image header, pass to the screen sharing window
                        App.Current.Dispatcher.Invoke(() => {
                            _screenSharingWindow?.ProcessImageHeader(headerText);
                        });
                        
                        ServerStatus = $"Receiving screen image from {args.ClientId}...";
                        return;
                    }
                    
                    // If we're expecting image data and this doesn't look like text, 
                    // assume it's binary image data
                    if (headerText.Any(c => c < 32 && c != '\r' && c != '\n' && c != '\t'))
                    {
                        // Likely binary data, pass to screen sharing window
                        App.Current.Dispatcher.Invoke(() => {
                            _screenSharingWindow?.ProcessImageData(args.Message.Data, args.Message.Data.Length);
                        });
                        
                        // Don't update status for every chunk
                        return;
                    }
                }
                
                // Regular text message
                var text = Encoding.UTF8.GetString(args.Message.Data);
                App.Current.Dispatcher.Invoke(() =>
                {
                    ServerStatus = $"Received from {args.ClientId}: {text}";
                });
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error processing message from client {ClientId}", args.ClientId);
                
                App.Current.Dispatcher.Invoke(() =>
                {
                    ServerStatus = $"Error processing message from {args.ClientId}: {ex.Message}";
                });
            }
        }
    }
}