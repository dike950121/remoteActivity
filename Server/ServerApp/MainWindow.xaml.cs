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

namespace ServerApp;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        StartTcpListener();
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
                    byte[] buffer = new byte[1024];
                    int bytesRead = stream.Read(buffer, 0, buffer.Length);
                    string message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    Dispatcher.Invoke(() =>
                    {
                        this.Title = $"Received: {message}";
                    });
                    client.Close();
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
}