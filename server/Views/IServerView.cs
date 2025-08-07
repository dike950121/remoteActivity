using System;
using System.Collections.Generic;

namespace server.Views
{
    /// <summary>
    /// Interface defining the contract for the server view
    /// </summary>
    public interface IServerView
    {
        /// <summary>
        /// Event raised when the user requests to start/stop the server
        /// </summary>
        event EventHandler<int> StartStopRequested;

        /// <summary>
        /// Event raised when the user requests to clear the log
        /// </summary>
        event EventHandler ClearLogRequested;

        /// <summary>
        /// Event raised when the user requests to copy the log
        /// </summary>
        event EventHandler CopyLogRequested;

        /// <summary>
        /// Updates the server status display
        /// </summary>
        /// <param name="isRunning">Whether the server is running</param>
        void UpdateServerStatus(bool isRunning);

        /// <summary>
        /// Updates the connected clients count display
        /// </summary>
        /// <param name="count">The number of connected clients</param>
        void UpdateConnectedClientsCount(int count);

        /// <summary>
        /// Updates the log messages display
        /// </summary>
        /// <param name="messages">The list of log messages</param>
        void UpdateLogMessages(IReadOnlyList<string> messages);

        /// <summary>
        /// Adds a single log message to the display
        /// </summary>
        /// <param name="message">The log message to add</param>
        void AddLogMessage(string message);

        /// <summary>
        /// Shows a message to the user
        /// </summary>
        /// <param name="message">The message to show</param>
        /// <param name="title">The title of the message</param>
        void ShowMessage(string message, string title);
    }
} 