using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Newtonsoft.Json;

namespace RemoteAccessServer.Models
{
    /// <summary>
    /// Represents a firewall command to be sent to a remote client
    /// </summary>
    public class FirewallCommand : INotifyPropertyChanged
    {
        private string _commandId;
        private string _action;
        private Dictionary<string, object> _parameters;
        private DateTime _timestamp;
        private string _clientId;
        private FirewallCommandStatus _status;
        private string _result;
        private string _errorMessage;

        public FirewallCommand()
        {
            _commandId = Guid.NewGuid().ToString();
            _action = string.Empty;
            _parameters = new Dictionary<string, object>();
            _timestamp = DateTime.Now;
            _clientId = string.Empty;
            _status = FirewallCommandStatus.Pending;
            _result = string.Empty;
            _errorMessage = string.Empty;
        }

        /// <summary>
        /// Gets or sets the unique command identifier
        /// </summary>
        public string CommandId
        {
            get => _commandId;
            set
            {
                _commandId = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the action to perform (SetState, AddRule, RemoveRule, GetRules, Reset, etc.)
        /// </summary>
        public string Action
        {
            get => _action;
            set
            {
                _action = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the command parameters
        /// </summary>
        public Dictionary<string, object> Parameters
        {
            get => _parameters;
            set
            {
                _parameters = value ?? new Dictionary<string, object>();
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the timestamp when the command was created
        /// </summary>
        public DateTime Timestamp
        {
            get => _timestamp;
            set
            {
                _timestamp = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(TimestampText));
            }
        }

        /// <summary>
        /// Gets or sets the target client identifier
        /// </summary>
        public string ClientId
        {
            get => _clientId;
            set
            {
                _clientId = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the command execution status
        /// </summary>
        public FirewallCommandStatus Status
        {
            get => _status;
            set
            {
                _status = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(StatusText));
            }
        }

        /// <summary>
        /// Gets or sets the command execution result
        /// </summary>
        public string Result
        {
            get => _result;
            set
            {
                _result = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the error message if command failed
        /// </summary>
        public string ErrorMessage
        {
            get => _errorMessage;
            set
            {
                _errorMessage = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets the formatted timestamp text
        /// </summary>
        public string TimestampText => Timestamp.ToString("yyyy-MM-dd HH:mm:ss");

        /// <summary>
        /// Gets the status text for UI display
        /// </summary>
        public string StatusText => Status.ToString();

        /// <summary>
        /// Gets a display-friendly description of the command
        /// </summary>
        public string Description
        {
            get
            {
                return Action switch
                {
                    "SetState" => $"Set firewall state: {(Parameters.ContainsKey("Enable") ? (bool)Parameters["Enable"] ? "Enable" : "Disable" : "Unknown")}",
                    "AddRule" => $"Add rule: {(Parameters.ContainsKey("Name") ? Parameters["Name"] : "Unknown")}",
                    "RemoveRule" => $"Remove rule: {(Parameters.ContainsKey("Name") ? Parameters["Name"] : "Unknown")}",
                    "GetRules" => "Get firewall rules",
                    "GetStatus" => "Get firewall status",
                    "Reset" => "Reset firewall to defaults",
                    "BlockIP" => $"Block IP: {(Parameters.ContainsKey("IPAddress") ? Parameters["IPAddress"] : "Unknown")}",
                    "AllowIP" => $"Allow IP: {(Parameters.ContainsKey("IPAddress") ? Parameters["IPAddress"] : "Unknown")}",
                    "BlockPort" => $"Block port: {(Parameters.ContainsKey("Port") ? Parameters["Port"] : "Unknown")}",
                    "AllowPort" => $"Allow port: {(Parameters.ContainsKey("Port") ? Parameters["Port"] : "Unknown")}",
                    _ => $"Unknown action: {Action}"
                };
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        /// <summary>
        /// Raises the PropertyChanged event
        /// </summary>
        /// <param name="propertyName">Name of the property that changed</param>
        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        /// <summary>
        /// Serializes the command to JSON format for transmission
        /// </summary>
        /// <returns>JSON representation of the command</returns>
        public string ToJson()
        {
            try
            {
                var commandData = new
                {
                    CommandId = this.CommandId,
                    Action = this.Action,
                    Parameters = this.Parameters,
                    Timestamp = this.Timestamp,
                    ClientId = this.ClientId
                };

                return JsonConvert.SerializeObject(commandData, Formatting.None);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Failed to serialize firewall command: {ex.Message}", ex);
            }
        }

        /// <summary>
        /// Deserializes a JSON string to a FirewallCommand object
        /// </summary>
        /// <param name="json">JSON string to deserialize</param>
        /// <returns>FirewallCommand object</returns>
        public static FirewallCommand? FromJson(string json)
        {
            try
            {
                if (string.IsNullOrWhiteSpace(json))
                    return null;

                var data = JsonConvert.DeserializeObject<dynamic>(json);
                if (data == null)
                    return null;

                var command = new FirewallCommand
                {
                    CommandId = data.CommandId ?? Guid.NewGuid().ToString(),
                    Action = data.Action ?? string.Empty,
                    Timestamp = data.Timestamp ?? DateTime.Now,
                    ClientId = data.ClientId ?? string.Empty
                };

                if (data.Parameters != null)
                {
                    command.Parameters = JsonConvert.DeserializeObject<Dictionary<string, object>>(data.Parameters.ToString()) ?? new Dictionary<string, object>();
                }

                return command;
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Failed to deserialize firewall command: {ex.Message}", ex);
            }
        }

        /// <summary>
        /// Creates a copy of the current command
        /// </summary>
        /// <returns>A new FirewallCommand instance with the same properties</returns>
        public FirewallCommand Clone()
        {
            return new FirewallCommand
            {
                CommandId = Guid.NewGuid().ToString(), // Generate new ID for the clone
                Action = this.Action,
                Parameters = new Dictionary<string, object>(this.Parameters),
                Timestamp = DateTime.Now, // Update timestamp for the clone
                ClientId = this.ClientId,
                Status = FirewallCommandStatus.Pending, // Reset status for the clone
                Result = string.Empty,
                ErrorMessage = string.Empty
            };
        }

        /// <summary>
        /// Validates the command configuration
        /// </summary>
        /// <returns>True if the command is valid, false otherwise</returns>
        public bool IsValid()
        {
            if (string.IsNullOrWhiteSpace(Action))
                return false;

            if (string.IsNullOrWhiteSpace(ClientId))
                return false;

            // Validate specific actions
            switch (Action)
            {
                case "SetState":
                    return Parameters.ContainsKey("Enable");
                case "AddRule":
                    return Parameters.ContainsKey("Name") && Parameters.ContainsKey("Direction") && Parameters.ContainsKey("Action");
                case "RemoveRule":
                    return Parameters.ContainsKey("Name");
                case "BlockIP":
                case "AllowIP":
                    return Parameters.ContainsKey("IPAddress");
                case "BlockPort":
                case "AllowPort":
                    return Parameters.ContainsKey("Port");
                case "GetRules":
                case "GetStatus":
                case "Reset":
                    return true; // These actions don't require additional parameters
                default:
                    return false;
            }
        }

        public override string ToString()
        {
            return $"{Action} - {Description} ({StatusText})";
        }
    }

    /// <summary>
    /// Represents the execution status of a firewall command
    /// </summary>
    public enum FirewallCommandStatus
    {
        /// <summary>
        /// Command is waiting to be sent
        /// </summary>
        Pending,

        /// <summary>
        /// Command has been sent to the client
        /// </summary>
        Sent,

        /// <summary>
        /// Command is being executed on the client
        /// </summary>
        Executing,

        /// <summary>
        /// Command executed successfully
        /// </summary>
        Success,

        /// <summary>
        /// Command execution failed
        /// </summary>
        Failed,

        /// <summary>
        /// Command execution timed out
        /// </summary>
        Timeout,

        /// <summary>
        /// Command was cancelled
        /// </summary>
        Cancelled
    }
}