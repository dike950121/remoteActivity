using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace RemoteAccessServer.Models
{
    /// <summary>
    /// Represents information about a connected client
    /// </summary>
    public class ClientInfo : INotifyPropertyChanged
    {
        private string _clientId;
        private string _ipAddress;
        private string _operatingSystem;
        private string _status;
        private DateTime _lastSeen;
        private int _ping;
        private string _computerName;
        private string _userName;
        private string _version;

        public string ClientId
        {
            get => _clientId;
            set
            {
                _clientId = value;
                OnPropertyChanged();
            }
        }

        public string IpAddress
        {
            get => _ipAddress;
            set
            {
                _ipAddress = value;
                OnPropertyChanged();
            }
        }

        public string OperatingSystem
        {
            get => _operatingSystem;
            set
            {
                _operatingSystem = value;
                OnPropertyChanged();
            }
        }

        public string Status
        {
            get => _status;
            set
            {
                _status = value;
                OnPropertyChanged();
            }
        }

        public DateTime LastSeen
        {
            get => _lastSeen;
            set
            {
                _lastSeen = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(LastSeenFormatted));
            }
        }

        public string LastSeenFormatted => LastSeen.ToString("yyyy-MM-dd HH:mm:ss");

        public int Ping
        {
            get => _ping;
            set
            {
                _ping = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(PingFormatted));
            }
        }

        public string PingFormatted => $"{Ping}ms";

        public string ComputerName
        {
            get => _computerName;
            set
            {
                _computerName = value;
                OnPropertyChanged();
            }
        }

        public string UserName
        {
            get => _userName;
            set
            {
                _userName = value;
                OnPropertyChanged();
            }
        }

        public string Version
        {
            get => _version;
            set
            {
                _version = value;
                OnPropertyChanged();
            }
        }

        public DateTime ConnectedAt { get; set; }
        public long BytesSent { get; set; }
        public long BytesReceived { get; set; }
        public int CommandsExecuted { get; set; }

        public ClientInfo()
        {
            _clientId = Guid.NewGuid().ToString("N")[..8].ToUpper();
            _status = "Connecting";
            ConnectedAt = DateTime.Now;
            LastSeen = DateTime.Now;
            _operatingSystem = string.Empty;
            _computerName = string.Empty;
            _userName = string.Empty;
            _version = string.Empty;
            _ipAddress = string.Empty;
        }

        public ClientInfo(string clientId, string ipAddress) : this()
        {
            ClientId = clientId;
            IpAddress = ipAddress;
        }

        /// <summary>
        /// Update the last seen timestamp
        /// </summary>
        public void UpdateLastSeen()
        {
            LastSeen = DateTime.Now;
        }

        /// <summary>
        /// Mark client as online
        /// </summary>
        public void SetOnline()
        {
            Status = "Online";
            UpdateLastSeen();
        }

        /// <summary>
        /// Mark client as offline
        /// </summary>
        public void SetOffline()
        {
            Status = "Offline";
        }

        /// <summary>
        /// Update ping value
        /// </summary>
        /// <param name="pingMs">Ping in milliseconds</param>
        public void UpdatePing(int pingMs)
        {
            Ping = pingMs;
        }

        /// <summary>
        /// Add to bytes sent counter
        /// </summary>
        /// <param name="bytes">Number of bytes sent</param>
        public void AddBytesSent(long bytes)
        {
            BytesSent += bytes;
        }

        /// <summary>
        /// Add to bytes received counter
        /// </summary>
        /// <param name="bytes">Number of bytes received</param>
        public void AddBytesReceived(long bytes)
        {
            BytesReceived += bytes;
        }

        /// <summary>
        /// Increment command execution counter
        /// </summary>
        public void IncrementCommandsExecuted()
        {
            CommandsExecuted++;
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public override string ToString()
        {
            return $"Client {ClientId} ({IpAddress}) - {Status}";
        }
    }
}