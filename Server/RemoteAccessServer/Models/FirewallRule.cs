using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace RemoteAccessServer.Models
{
    /// <summary>
    /// Represents a Windows Firewall rule configuration
    /// </summary>
    public class FirewallRule : INotifyPropertyChanged
    {
        private string _name;
        private string _direction;
        private string _action;
        private string _protocol;
        private string _localPort;
        private string _remotePort;
        private string _localAddress;
        private string _remoteAddress;
        private string _program;
        private string _service;
        private string _description;
        private bool _enabled;
        private string _profile;
        private DateTime _createdDate;

        public FirewallRule()
        {
            _name = string.Empty;
            _direction = "Inbound";
            _action = "Allow";
            _protocol = "TCP";
            _localPort = string.Empty;
            _remotePort = string.Empty;
            _localAddress = string.Empty;
            _remoteAddress = string.Empty;
            _program = string.Empty;
            _service = string.Empty;
            _description = string.Empty;
            _enabled = true;
            _profile = "Any";
            _createdDate = DateTime.Now;
        }

        /// <summary>
        /// Gets or sets the name of the firewall rule
        /// </summary>
        public string Name
        {
            get => _name;
            set
            {
                _name = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the direction of the rule (Inbound/Outbound)
        /// </summary>
        public string Direction
        {
            get => _direction;
            set
            {
                _direction = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the action of the rule (Allow/Block)
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
        /// Gets or sets the protocol (TCP/UDP/Any)
        /// </summary>
        public string Protocol
        {
            get => _protocol;
            set
            {
                _protocol = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the local port(s)
        /// </summary>
        public string LocalPort
        {
            get => _localPort;
            set
            {
                _localPort = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the remote port(s)
        /// </summary>
        public string RemotePort
        {
            get => _remotePort;
            set
            {
                _remotePort = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the local address(es)
        /// </summary>
        public string LocalAddress
        {
            get => _localAddress;
            set
            {
                _localAddress = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the remote address(es)
        /// </summary>
        public string RemoteAddress
        {
            get => _remoteAddress;
            set
            {
                _remoteAddress = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the program path
        /// </summary>
        public string Program
        {
            get => _program;
            set
            {
                _program = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the service name
        /// </summary>
        public string Service
        {
            get => _service;
            set
            {
                _service = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the description of the rule
        /// </summary>
        public string Description
        {
            get => _description;
            set
            {
                _description = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets whether the rule is enabled
        /// </summary>
        public bool Enabled
        {
            get => _enabled;
            set
            {
                _enabled = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the firewall profile (Domain/Private/Public/Any)
        /// </summary>
        public string Profile
        {
            get => _profile;
            set
            {
                _profile = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the date when the rule was created
        /// </summary>
        public DateTime CreatedDate
        {
            get => _createdDate;
            set
            {
                _createdDate = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets a formatted string representation of the rule
        /// </summary>
        public string DisplayText => $"{Name} - {Direction} {Action} ({Protocol})";

        /// <summary>
        /// Gets the status text for UI display
        /// </summary>
        public string StatusText => Enabled ? "Enabled" : "Disabled";

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
        /// Creates a copy of the current firewall rule
        /// </summary>
        /// <returns>A new FirewallRule instance with the same properties</returns>
        public FirewallRule Clone()
        {
            return new FirewallRule
            {
                Name = this.Name,
                Direction = this.Direction,
                Action = this.Action,
                Protocol = this.Protocol,
                LocalPort = this.LocalPort,
                RemotePort = this.RemotePort,
                LocalAddress = this.LocalAddress,
                RemoteAddress = this.RemoteAddress,
                Program = this.Program,
                Service = this.Service,
                Description = this.Description,
                Enabled = this.Enabled,
                Profile = this.Profile,
                CreatedDate = this.CreatedDate
            };
        }

        /// <summary>
        /// Validates the firewall rule configuration
        /// </summary>
        /// <returns>True if the rule is valid, false otherwise</returns>
        public bool IsValid()
        {
            if (string.IsNullOrWhiteSpace(Name))
                return false;

            if (Direction != "Inbound" && Direction != "Outbound")
                return false;

            if (Action != "Allow" && Action != "Block")
                return false;

            return true;
        }

        public override string ToString()
        {
            return DisplayText;
        }
    }
}