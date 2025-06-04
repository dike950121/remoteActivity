using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Threading.Tasks;
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
        private FirewallManager _firewallManager;
        private ObservableCollection<ClientInfo> _clients;
        private ObservableCollection<string> _recentActivity;
        private ObservableCollection<FirewallRule> _firewallRules;
        private string _selectedClientId;

        public MainWindow()
        {
            _serverManager = new RemoteAccessServer.Core.ServerManager();
            _firewallManager = new FirewallManager();
            _clients = new ObservableCollection<ClientInfo>();
            _recentActivity = new ObservableCollection<string>();
            _firewallRules = new ObservableCollection<FirewallRule>();
            InitializeComponent();
            InitializeComponents();
        }

        private void InitializeComponents()
        {
            // Bind data to UI elements
            ClientsDataGrid.ItemsSource = _clients;
            RecentActivityList.ItemsSource = _recentActivity;
            FirewallRulesDataGrid.ItemsSource = _firewallRules;
            FirewallClientComboBox.ItemsSource = _clients;
            FirewallClientComboBox.DisplayMemberPath = "ClientId";
            FirewallClientComboBox.SelectedValuePath = "ClientId";

            // Initialize protocol combobox
            ProtocolComboBox.SelectedIndex = 0; // Default to TCP

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
            FirewallView.Visibility = Visibility.Collapsed;
            PlaceholderView.Visibility = Visibility.Collapsed;

            switch (section)
            {
                case "Dashboard":
                    DashboardView.Visibility = Visibility.Visible;
                    break;
                case "Clients":
                    ClientsView.Visibility = Visibility.Visible;
                    break;
                case "Firewall":
                    FirewallView.Visibility = Visibility.Visible;
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

        #region Firewall Event Handlers

        /// <summary>
        /// Handles firewall client selection change
        /// </summary>
        private async void FirewallClientComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (FirewallClientComboBox.SelectedValue is string clientId)
            {
                _selectedClientId = clientId;
                await RefreshFirewallRules();
                await RefreshFirewallStatus();
            }
        }

        /// <summary>
        /// Enables firewall for selected client
        /// </summary>
        private async void EnableFirewall_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.SetFirewallStateAsync(_selectedClientId, true);
                AddRecentActivity($"Firewall enabled for client {_selectedClientId}");
                await RefreshFirewallStatus();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to enable firewall: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to enable firewall for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Disables firewall for selected client
        /// </summary>
        private async void DisableFirewall_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.SetFirewallStateAsync(_selectedClientId, false);
                AddRecentActivity($"Firewall disabled for client {_selectedClientId}");
                await RefreshFirewallStatus();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to disable firewall: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to disable firewall for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Resets firewall to default settings for selected client
        /// </summary>
        private async void ResetFirewall_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var result = MessageBox.Show("Are you sure you want to reset the firewall to default settings? This will remove all custom rules.", 
                "Confirm Reset", MessageBoxButton.YesNo, MessageBoxImage.Question);
            
            if (result == MessageBoxResult.Yes)
            {
                try
                {
                    await _firewallManager.ResetFirewallAsync(_selectedClientId);
                    AddRecentActivity($"Firewall reset for client {_selectedClientId}");
                    await RefreshFirewallRules();
                    await RefreshFirewallStatus();
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Failed to reset firewall: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    Logger.LogError($"Failed to reset firewall for client {_selectedClientId}: {ex}");
                }
            }
        }

        /// <summary>
        /// Adds a new firewall rule
        /// </summary>
        private async void AddRule_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(RuleNameTextBox.Text))
            {
                MessageBox.Show("Please enter a rule name.", "Invalid Input", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                var rule = new FirewallRule
                {
                    Name = RuleNameTextBox.Text.Trim(),
                    Direction = DirectionComboBox.SelectedItem?.ToString() ?? "Inbound",
                    Action = ActionComboBox.SelectedItem?.ToString() ?? "Allow",
                    Protocol = ProtocolComboBox.SelectedItem?.ToString() ?? "TCP",
                    LocalPort = string.IsNullOrWhiteSpace(LocalPortsTextBox.Text) ? null : LocalPortsTextBox.Text.Trim(),
                    RemotePort = string.IsNullOrWhiteSpace(RemotePortsTextBox.Text) ? null : RemotePortsTextBox.Text.Trim(),
                    LocalAddress = string.IsNullOrWhiteSpace(LocalAddressesTextBox.Text) ? null : LocalAddressesTextBox.Text.Trim(),
                    RemoteAddress = string.IsNullOrWhiteSpace(RemoteAddressesTextBox.Text) ? null : RemoteAddressesTextBox.Text.Trim(),
                    Program = string.IsNullOrWhiteSpace(ProgramTextBox.Text) ? null : ProgramTextBox.Text.Trim(),
                    Service = string.IsNullOrWhiteSpace(ServiceTextBox.Text) ? null : ServiceTextBox.Text.Trim(),
                    Description = string.IsNullOrWhiteSpace(DescriptionTextBox.Text) ? null : DescriptionTextBox.Text.Trim(),
                    Enabled = EnabledCheckBox.IsChecked ?? true,
                    Profile = ProfileComboBox.SelectedItem?.ToString() ?? "Any"
                };

                await _firewallManager.AddFirewallRuleAsync(_selectedClientId, rule);
                AddRecentActivity($"Firewall rule '{rule.Name}' added for client {_selectedClientId}");
                await RefreshFirewallRules();
                ClearRuleInputs();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to add firewall rule: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to add firewall rule for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Removes selected firewall rule
        /// </summary>
        private async void RemoveRule_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (FirewallRulesDataGrid.SelectedItem is FirewallRule selectedRule)
            {
                var result = MessageBox.Show($"Are you sure you want to remove the rule '{selectedRule.Name}'?", 
                    "Confirm Removal", MessageBoxButton.YesNo, MessageBoxImage.Question);
                
                if (result == MessageBoxResult.Yes)
                {
                    try
                    {
                        await _firewallManager.RemoveFirewallRuleAsync(_selectedClientId, selectedRule.Name);
                        AddRecentActivity($"Firewall rule '{selectedRule.Name}' removed for client {_selectedClientId}");
                        await RefreshFirewallRules();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show($"Failed to remove firewall rule: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        Logger.LogError($"Failed to remove firewall rule for client {_selectedClientId}: {ex}");
                    }
                }
            }
            else
            {
                MessageBox.Show("Please select a rule to remove.", "No Rule Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        /// <summary>
        /// Refreshes firewall rules list
        /// </summary>
        private async void RefreshRules_Click(object sender, RoutedEventArgs e)
        {
            await RefreshFirewallRules();
        }

        /// <summary>
        /// Refreshes firewall status
        /// </summary>
        private async void RefreshFirewall_Click(object sender, RoutedEventArgs e)
        {
            await RefreshFirewallStatus();
        }

        /// <summary>
        /// Blocks an IP address
        /// </summary>
        private async void BlockIp_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(IpAddressTextBox.Text))
            {
                MessageBox.Show("Please enter an IP address to block.", "Invalid Input", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.BlockIpAddressAsync(_selectedClientId, IpAddressTextBox.Text.Trim());
                AddRecentActivity($"IP address {IpAddressTextBox.Text.Trim()} blocked for client {_selectedClientId}");
                await RefreshFirewallRules();
                IpAddressTextBox.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to block IP address: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to block IP address for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Allows an IP address
        /// </summary>
        private async void AllowIp_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(IpAddressTextBox.Text))
            {
                MessageBox.Show("Please enter an IP address to allow.", "Invalid Input", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.AllowIpAddressAsync(_selectedClientId, IpAddressTextBox.Text.Trim());
                AddRecentActivity($"IP address {IpAddressTextBox.Text.Trim()} allowed for client {_selectedClientId}");
                await RefreshFirewallRules();
                IpAddressTextBox.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to allow IP address: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to allow IP address for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Blocks a port
        /// </summary>
        private async void BlockPort_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(PortTextBox.Text) || !int.TryParse(PortTextBox.Text.Trim(), out int port) || port < 1 || port > 65535)
            {
                MessageBox.Show("Please enter a valid port number (1-65535).", "Invalid Input", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.BlockPortAsync(_selectedClientId, port);
                AddRecentActivity($"Port {port} blocked for client {_selectedClientId}");
                await RefreshFirewallRules();
                PortTextBox.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to block port: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to block port for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Allows a port
        /// </summary>
        private async void AllowPort_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrEmpty(_selectedClientId))
            {
                MessageBox.Show("Please select a client first.", "No Client Selected", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(PortTextBox.Text) || !int.TryParse(PortTextBox.Text.Trim(), out int port) || port < 1 || port > 65535)
            {
                MessageBox.Show("Please enter a valid port number (1-65535).", "Invalid Input", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                await _firewallManager.AllowPortAsync(_selectedClientId, port);
                AddRecentActivity($"Port {port} allowed for client {_selectedClientId}");
                await RefreshFirewallRules();
                PortTextBox.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to allow port: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Logger.LogError($"Failed to allow port for client {_selectedClientId}: {ex}");
            }
        }

        #endregion

        #region Firewall Helper Methods

        /// <summary>
        /// Refreshes the firewall rules list for the selected client
        /// </summary>
        private async Task RefreshFirewallRules()
        {
            if (string.IsNullOrEmpty(_selectedClientId))
                return;

            try
            {
                var rules = await _firewallManager.GetFirewallRulesAsync(_selectedClientId);
                _firewallRules.Clear();
                foreach (var rule in rules)
                {
                    _firewallRules.Add(rule);
                }
            }
            catch (Exception ex)
            {
                Logger.LogError($"Failed to refresh firewall rules for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Refreshes the firewall status for the selected client
        /// </summary>
        private async Task RefreshFirewallStatus()
        {
            if (string.IsNullOrEmpty(_selectedClientId))
                return;

            try
            {
                var status = await _firewallManager.GetFirewallStatusAsync(_selectedClientId);
                // Update firewall status display (you can add UI elements to show this)
                AddRecentActivity($"Firewall status refreshed for client {_selectedClientId}");
            }
            catch (Exception ex)
            {
                Logger.LogError($"Failed to refresh firewall status for client {_selectedClientId}: {ex}");
            }
        }

        /// <summary>
        /// Clears all rule input fields
        /// </summary>
        private void ClearRuleInputs()
        {
            RuleNameTextBox.Clear();
            LocalPortsTextBox.Clear();
            RemotePortsTextBox.Clear();
            LocalAddressesTextBox.Clear();
            RemoteAddressesTextBox.Clear();
            ProgramTextBox.Clear();
            ServiceTextBox.Clear();
            DescriptionTextBox.Clear();
            DirectionComboBox.SelectedIndex = 0;
            ActionComboBox.SelectedIndex = 0;
            ProtocolComboBox.SelectedIndex = 0;
            ProfileComboBox.SelectedIndex = 0;
            EnabledCheckBox.IsChecked = true;
        }

        #endregion

        protected override void OnClosed(EventArgs e)
        {
            _serverManager?.Dispose();
            base.OnClosed(e);
        }
    }
}