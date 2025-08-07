using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

namespace server.Views
{
    /// <summary>
    /// Interaction logic for ServerView.xaml
    /// </summary>
    public partial class ServerView : Window, IServerView
    {
        /// <summary>
        /// Event raised when the user requests to start/stop the server
        /// </summary>
        public event EventHandler<int>? StartStopRequested;

        /// <summary>
        /// Event raised when the user requests to clear the log
        /// </summary>
        public event EventHandler? ClearLogRequested;

        /// <summary>
        /// Event raised when the user requests to copy the log
        /// </summary>
        public event EventHandler? CopyLogRequested;

        /// <summary>
        /// Initializes a new instance of the ServerView
        /// </summary>
        public ServerView()
        {
            InitializeComponent();
            
            // Wire up event handlers
            StartStopButton.Click += StartStopButton_Click;
            ClearLogButton.Click += ClearLogButton_Click;
            CopyLogButton.Click += CopyLogButton_Click;
            
            // Initialize status
            UpdateServerStatus(false);
            UpdateConnectedClientsCount(0);
        }

        /// <summary>
        /// Handles the start/stop button click event
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void StartStopButton_Click(object sender, RoutedEventArgs e)
        {
            if (int.TryParse(PortTextBox.Text, out int port))
            {
                StartStopRequested?.Invoke(this, port);
            }
            else
            {
                ShowMessage("Please enter a valid port number", "Invalid Port");
            }
        }

        /// <summary>
        /// Handles the clear log button click event
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void ClearLogButton_Click(object sender, RoutedEventArgs e)
        {
            ClearLogRequested?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Handles the copy log button click event
        /// </summary>
        /// <param name="sender">Event sender</param>
        /// <param name="e">Event arguments</param>
        private void CopyLogButton_Click(object sender, RoutedEventArgs e)
        {
            CopyLogRequested?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Updates the server status display
        /// </summary>
        /// <param name="isRunning">Whether the server is running</param>
        public void UpdateServerStatus(bool isRunning)
        {
            Dispatcher.Invoke(() =>
            {
                if (isRunning)
                {
                    StatusLabel.Text = "Running";
                    StatusLabel.Foreground = System.Windows.Media.Brushes.Green;
                    StartStopButton.Content = "Stop Server";
                    StatusBarText.Text = "Server is running - MVC Architecture";
                }
                else
                {
                    StatusLabel.Text = "Stopped";
                    StatusLabel.Foreground = System.Windows.Media.Brushes.Red;
                    StartStopButton.Content = "Start Server";
                    StatusBarText.Text = "Server is stopped - MVC Architecture";
                }
            });
        }

        /// <summary>
        /// Updates the connected clients count display
        /// </summary>
        /// <param name="count">The number of connected clients</param>
        public void UpdateConnectedClientsCount(int count)
        {
            Dispatcher.Invoke(() =>
            {
                ClientsLabel.Text = count.ToString();
            });
        }

        /// <summary>
        /// Updates the log messages display
        /// </summary>
        /// <param name="messages">The list of log messages</param>
        public void UpdateLogMessages(IReadOnlyList<string> messages)
        {
            Dispatcher.Invoke(() =>
            {
                LogTextBox.Text = string.Join(Environment.NewLine, messages);
                LogTextBox.ScrollToEnd();
            });
        }

        /// <summary>
        /// Adds a single log message to the display
        /// </summary>
        /// <param name="message">The log message to add</param>
        public void AddLogMessage(string message)
        {
            Dispatcher.Invoke(() =>
            {
                if (!string.IsNullOrEmpty(LogTextBox.Text))
                {
                    LogTextBox.Text += Environment.NewLine;
                }
                LogTextBox.Text += message;
                LogTextBox.ScrollToEnd();
            });
        }

        /// <summary>
        /// Shows a message to the user
        /// </summary>
        /// <param name="message">The message to show</param>
        /// <param name="title">The title of the message</param>
        public void ShowMessage(string message, string title)
        {
            Dispatcher.Invoke(() =>
            {
                MessageBox.Show(this, message, title, MessageBoxButton.OK, 
                    title.ToLower() == "error" ? MessageBoxImage.Error : 
                    title.ToLower() == "success" ? MessageBoxImage.Information : 
                    MessageBoxImage.Information);
            });
        }
    }
} 