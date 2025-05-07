using System;

namespace RemoteServer.Models
{
    public class Message
    {
        public const int MaxMessageSize = 4096;
        
        public MessageType Type { get; set; }
        public byte[] Data { get; set; } = Array.Empty<byte>();
        public int DataLength => Data.Length;
    }
}