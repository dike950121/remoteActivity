using System;

namespace RemoteAccessServer.Models
{
    public class ClientDisconnectedEventArgs : EventArgs
    {
        public string ClientId { get; }
        public string IpAddress { get; }

        public ClientDisconnectedEventArgs(ClientInfo clientInfo)
        {
            ClientId = clientInfo.ClientId;
            IpAddress = clientInfo.IpAddress;
        }
    }
}