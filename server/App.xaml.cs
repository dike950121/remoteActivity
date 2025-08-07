using System.Windows;
using server.Models;
using server.Views;
using server.Controllers;

namespace server
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private ServerModel? _model;
        private ServerView? _view;
        private ServerController? _controller;

        /// <summary>
        /// Handles the application startup event
        /// </summary>
        /// <param name="e">Startup event arguments</param>
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            try
            {
                // Initialize MVC components
                _model = new ServerModel();
                _view = new ServerView();
                _controller = new ServerController(_model, _view);

                // Show the main window
                _view.Show();
            }
            catch (System.Exception ex)
            {
                MessageBox.Show($"Failed to initialize application: {ex.Message}", "Startup Error",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                Shutdown();
            }
        }

        /// <summary>
        /// Handles the application exit event
        /// </summary>
        /// <param name="e">Exit event arguments</param>
        protected override void OnExit(ExitEventArgs e)
        {
            // Clean up MVC components
            _controller?.Dispose();
            _view?.Close();
            
            base.OnExit(e);
        }
    }
}

