using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace RemoteAccessServer.Models
{
    /// <summary>
    /// Represents the current status of Windows Firewall on a remote client
    /// </summary>
    public class FirewallStatus : INotifyPropertyChanged
    {
        private string _clientId;
        private bool _isEnabled;
        private FirewallProfile _domainProfile;
        private FirewallProfile _privateProfile;
        private FirewallProfile _publicProfile;
        private DateTime _lastUpdated;
        private string _version;
        private int _totalRules;
        private int _activeRules;

        public FirewallStatus()
        {
            _clientId = string.Empty;
            _isEnabled = false;
            _domainProfile = new FirewallProfile();
            _privateProfile = new FirewallProfile();
            _publicProfile = new FirewallProfile();
            _lastUpdated = DateTime.Now;
            _version = string.Empty;
            _totalRules = 0;
            _activeRules = 0;
        }

        /// <summary>
        /// Gets or sets the client identifier
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
        /// Gets or sets whether the firewall is enabled
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set
            {
                _isEnabled = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(StatusText));
            }
        }

        /// <summary>
        /// Gets or sets the domain profile configuration
        /// </summary>
        public FirewallProfile DomainProfile
        {
            get => _domainProfile;
            set
            {
                _domainProfile = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the private profile configuration
        /// </summary>
        public FirewallProfile PrivateProfile
        {
            get => _privateProfile;
            set
            {
                _privateProfile = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the public profile configuration
        /// </summary>
        public FirewallProfile PublicProfile
        {
            get => _publicProfile;
            set
            {
                _publicProfile = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the last update timestamp
        /// </summary>
        public DateTime LastUpdated
        {
            get => _lastUpdated;
            set
            {
                _lastUpdated = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(LastUpdatedText));
            }
        }

        /// <summary>
        /// Gets or sets the firewall version
        /// </summary>
        public string Version
        {
            get => _version;
            set
            {
                _version = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the total number of firewall rules
        /// </summary>
        public int TotalRules
        {
            get => _totalRules;
            set
            {
                _totalRules = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the number of active firewall rules
        /// </summary>
        public int ActiveRules
        {
            get => _activeRules;
            set
            {
                _activeRules = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets the status text for UI display
        /// </summary>
        public string StatusText => IsEnabled ? "Enabled" : "Disabled";

        /// <summary>
        /// Gets the formatted last updated text
        /// </summary>
        public string LastUpdatedText => LastUpdated.ToString("yyyy-MM-dd HH:mm:ss");

        /// <summary>
        /// Gets the overall security level based on profile configurations
        /// </summary>
        public string SecurityLevel
        {
            get
            {
                if (!IsEnabled)
                    return "Disabled";

                int enabledProfiles = 0;
                if (DomainProfile.Enabled) enabledProfiles++;
                if (PrivateProfile.Enabled) enabledProfiles++;
                if (PublicProfile.Enabled) enabledProfiles++;

                return enabledProfiles switch
                {
                    3 => "High",
                    2 => "Medium",
                    1 => "Low",
                    _ => "None"
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
    }

    /// <summary>
    /// Represents a Windows Firewall profile (Domain, Private, or Public)
    /// </summary>
    public class FirewallProfile : INotifyPropertyChanged
    {
        private bool _enabled;
        private string _inboundAction;
        private string _outboundAction;
        private bool _notificationsEnabled;
        private bool _unicastResponseToMulticast;
        private bool _logAllowed;
        private bool _logBlocked;
        private string _logFilePath;
        private int _logMaxSize;

        public FirewallProfile()
        {
            _enabled = true;
            _inboundAction = "Block";
            _outboundAction = "Allow";
            _notificationsEnabled = true;
            _unicastResponseToMulticast = true;
            _logAllowed = false;
            _logBlocked = false;
            _logFilePath = string.Empty;
            _logMaxSize = 4096;
        }

        /// <summary>
        /// Gets or sets whether this profile is enabled
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
        /// Gets or sets the default inbound action (Allow/Block)
        /// </summary>
        public string InboundAction
        {
            get => _inboundAction;
            set
            {
                _inboundAction = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the default outbound action (Allow/Block)
        /// </summary>
        public string OutboundAction
        {
            get => _outboundAction;
            set
            {
                _outboundAction = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets whether notifications are enabled
        /// </summary>
        public bool NotificationsEnabled
        {
            get => _notificationsEnabled;
            set
            {
                _notificationsEnabled = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets whether unicast response to multicast is allowed
        /// </summary>
        public bool UnicastResponseToMulticast
        {
            get => _unicastResponseToMulticast;
            set
            {
                _unicastResponseToMulticast = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets whether allowed connections are logged
        /// </summary>
        public bool LogAllowed
        {
            get => _logAllowed;
            set
            {
                _logAllowed = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets whether blocked connections are logged
        /// </summary>
        public bool LogBlocked
        {
            get => _logBlocked;
            set
            {
                _logBlocked = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the log file path
        /// </summary>
        public string LogFilePath
        {
            get => _logFilePath;
            set
            {
                _logFilePath = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Gets or sets the maximum log file size in KB
        /// </summary>
        public int LogMaxSize
        {
            get => _logMaxSize;
            set
            {
                _logMaxSize = value;
                OnPropertyChanged();
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
    }
}