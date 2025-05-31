using System;

namespace RemoteAccessServer.Models
{
    public class CommandExecutedEventArgs : EventArgs
    {
        public string ClientId { get; }
        public string Command { get; }
        public string Response { get; }

        public CommandExecutedEventArgs(string clientId, string command, string response)
        {
            ClientId = clientId;
            Command = command;
            Response = response;
        }
    }
}