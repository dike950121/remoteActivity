using System;
using System.Windows;
using RemoteAccessServer.Core;

namespace RemoteAccessServer
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            
            // Initialize logging
            Logger.Initialize();
            Logger.Log("Application starting...");
            
            // Handle unhandled exceptions
            this.DispatcherUnhandledException += App_DispatcherUnhandledException;
            AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
        }

        private void App_DispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            Logger.LogError($"Unhandled UI exception: {e.Exception}");
            MessageBox.Show($"An unexpected error occurred: {e.Exception.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            e.Handled = true;
        }

        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Logger.LogError($"Unhandled domain exception: {e.ExceptionObject}");
            if (e.IsTerminating)
            {
                Logger.Log("Application is terminating due to unhandled exception.");
            }
        }

        protected override void OnExit(ExitEventArgs e)
        {
            Logger.Log("Application shutting down...");
            base.OnExit(e);
        }
    }
}