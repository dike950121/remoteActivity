using System.Net.Sockets;
using Newtonsoft.Json;

namespace RemoteActivityServer.Models
{
    /// <summary>
    /// Represents a connected spy bot client
    /// </summary>
    public class ClientConnection
    {
        /// <summary>
        /// Unique identifier for the client
        /// </summary>
        public string ClientId { get; set; } = string.Empty;
        
        /// <summary>
        /// Display name for the client
        /// </summary>
        public string DisplayName { get; set; } = string.Empty;
        
        /// <summary>
        /// IP address of the connected client
        /// </summary>
        public string IpAddress { get; set; } = string.Empty;
        
        /// <summary>
        /// TCP client socket connection
        /// </summary>
        public TcpClient? TcpClient { get; set; }
        
        /// <summary>
        /// Network stream for communication
        /// </summary>
        public NetworkStream? Stream { get; set; }
        
        /// <summary>
        /// Timestamp when client connected
        /// </summary>
        public DateTime ConnectedAt { get; set; } = DateTime.Now;
        
        /// <summary>
        /// Last time data was received from client
        /// </summary>
        public DateTime LastDataReceived { get; set; } = DateTime.Now;
        
        /// <summary>
        /// Current connection status
        /// </summary>
        public ConnectionStatus Status { get; set; } = ConnectionStatus.Connected;
        
        /// <summary>
        /// Latest system data received from client
        /// </summary>
        public SystemData? LatestData { get; set; }
        
        /// <summary>
        /// Check if the connection is still alive
        /// </summary>
        public bool IsConnected
        {
            get
            {
                if (TcpClient == null) return false;
                try
                {
                    return TcpClient.Connected && TcpClient.Client.Poll(0, SelectMode.SelectRead) == false;
                }
                catch
                {
                    return false;
                }
            }
        }
        
        /// <summary>
        /// Get connection duration
        /// </summary>
        public TimeSpan ConnectionDuration => DateTime.Now - ConnectedAt;
    }
    
    /// <summary>
    /// Connection status enumeration
    /// </summary>
    public enum ConnectionStatus
    {
        Connected,
        Disconnected,
        Timeout,
        Error
    }
    
    /// <summary>
    /// System data received from spy bot client
    /// </summary>
    public class SystemData
    {
        [JsonProperty("client_id")]
        public string ClientId { get; set; } = string.Empty;
        
        [JsonProperty("message_type")]
        public string MessageType { get; set; } = string.Empty;
        
        [JsonProperty("timestamp")]
        public string Timestamp { get; set; } = string.Empty;
        
        [JsonProperty("data")]
        public DataContent? Data { get; set; }
        
        /// <summary>
        /// Parse timestamp to DateTime
        /// </summary>
        public DateTime GetTimestamp()
        {
            if (long.TryParse(Timestamp, out long unixTimestamp))
            {
                return DateTimeOffset.FromUnixTimeSeconds(unixTimestamp).DateTime;
            }
            return DateTime.Now;
        }
    }
    
    /// <summary>
    /// Content of system data
    /// </summary>
    public class DataContent
    {
        [JsonProperty("os_info")]
        public object? OsInfo { get; set; }
        
        [JsonProperty("hardware_info")]
        public object? HardwareInfo { get; set; }
        
        [JsonProperty("performance")]
        public object? Performance { get; set; }
        
        [JsonProperty("processes")]
        public ProcessInfo[]? Processes { get; set; }
        
        [JsonProperty("network_connections")]
        public NetworkConnection[]? NetworkConnections { get; set; }
    }
    
    /// <summary>
    /// Process information from client
    /// </summary>
    public class ProcessInfo
    {
        [JsonProperty("pid")]
        public int Pid { get; set; }
        
        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;
        
        [JsonProperty("memory_usage")]
        public long MemoryUsage { get; set; }
    }
    
    /// <summary>
    /// Network connection information from client
    /// </summary>
    public class NetworkConnection
    {
        [JsonProperty("protocol")]
        public string Protocol { get; set; } = string.Empty;
        
        [JsonProperty("local_address")]
        public string LocalAddress { get; set; } = string.Empty;
        
        [JsonProperty("local_port")]
        public int LocalPort { get; set; }
        
        [JsonProperty("remote_address")]
        public string RemoteAddress { get; set; } = string.Empty;
        
        [JsonProperty("remote_port")]
        public int RemotePort { get; set; }
        
        [JsonProperty("state")]
        public string State { get; set; } = string.Empty;
    }
} 