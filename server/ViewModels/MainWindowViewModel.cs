using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RemoteServer.Models;
using RemoteServer.Services;
using System.Text;

namespace RemoteServer.ViewModels
{
    public partial class MainWindowViewModel : ObservableObject
    {
        private readonly NetworkService _networkService;
        
        [ObservableProperty]
        private bool _isServerRunning;
        
        [ObservableProperty]
        private string _serverStatus = "Server Stopped";

        [ObservableProperty]
        private string? _selectedClientId;

        [ObservableProperty]
        private string _commandText = string.Empty;

        public ObservableCollection<ConnectedClient> ConnectedClients { get; } = new();

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
            if (string.IsNullOrEmpty(SelectedClientId) || string.IsNullOrEmpty(CommandText))
                return;

            var message = new Message
            {
                Type = MessageType.Command,
                Data = Encoding.UTF8.GetBytes(CommandText)
            };

            await _networkService.SendMessageAsync(SelectedClientId, message);
            CommandText = string.Empty;
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
            var text = Encoding.UTF8.GetString(args.Message.Data);
            App.Current.Dispatcher.Invoke(() =>
            {
                ServerStatus = $"Received from {args.ClientId}: {text}";
            });
        }
    }
}