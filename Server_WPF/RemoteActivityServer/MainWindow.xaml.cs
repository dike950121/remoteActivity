using System.Windows;
using RemoteActivityServer.ViewModels;

namespace RemoteActivityServer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        /// <summary>
        /// Constructor for MainWindow
        /// </summary>
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainViewModel();
        }
        
        /// <summary>
        /// Handle window closing to ensure proper cleanup
        /// </summary>
        /// <param name="e">Cancel event args</param>
        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            if (DataContext is MainViewModel viewModel)
            {
                // Stop server if running before closing
                if (viewModel.IsServerRunning)
                {
                    var result = MessageBox.Show(
                        "The server is currently running. Do you want to stop it and close the application?",
                        "Server Running",
                        MessageBoxButton.YesNo,
                        MessageBoxImage.Question);
                    
                    if (result == MessageBoxResult.No)
                    {
                        e.Cancel = true;
                        return;
                    }
                    
                    // Stop server asynchronously
                    Task.Run(() =>
                    {
                        viewModel.ToggleServerCommand.Execute(null);
                    }).Wait(5000); // Wait up to 5 seconds for cleanup
                }
            }
            
            base.OnClosing(e);
        }
    }
}