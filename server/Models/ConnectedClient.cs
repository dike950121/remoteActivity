using System;
using System.Net.Sockets;

namespace RemoteServer.Models
{
    public class ConnectedClient
    {
        public string Id { get; } = Guid.NewGuid().ToString();
        public string IpAddress { get; }
        public DateTime ConnectedAt { get; }
        public TcpClient TcpClient { get; }
        public NetworkStream Stream { get; }

        public ConnectedClient(TcpClient client)
        {
            TcpClient = client;
            Stream = client.GetStream();
            IpAddress = ((System.Net.IPEndPoint)client.Client.RemoteEndPoint!).Address.ToString();
            ConnectedAt = DateTime.UtcNow;
        }
    }
}