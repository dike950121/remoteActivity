using System;

namespace RemoteAccessServer.Models
{
    public class ClientConnectedEventArgs : EventArgs
    {
        public string ClientId { get; }
        public string IpAddress { get; }

        public ClientConnectedEventArgs(ClientInfo clientInfo)
        {
            ClientId = clientInfo.ClientId;
            IpAddress = clientInfo.IpAddress;
        }
    }
}