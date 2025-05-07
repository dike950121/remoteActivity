using System;
using System.Windows;
using Microsoft.Extensions.DependencyInjection;
using RemoteServer.Services;
using RemoteServer.ViewModels;
using Serilog;
using Serilog.Events;
using System.IO;

namespace RemoteServer
{
    public partial class App : Application
    {
        private ServiceProvider _serviceProvider = null!;
        private const string LogDirectory = "logs";

        public App()
        {
            var services = new ServiceCollection();
            ConfigureServices(services);
            _serviceProvider = services.BuildServiceProvider();

            // Ensure log directory exists
            if (!Directory.Exists(LogDirectory))
            {
                Directory.CreateDirectory(LogDirectory);
            }

            // Configure logging with more detailed format and better file management
            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Debug()
                .MinimumLevel.Override("Microsoft", LogEventLevel.Information)
                .Enrich.FromLogContext()
                .Enrich.WithThreadId()
                .WriteTo.File(
                    Path.Combine(LogDirectory, "server_log_.txt"),
                    rollingInterval: RollingInterval.Day,
                    outputTemplate: "{Timestamp:yyyy-MM-dd HH:mm:ss.fff} [{Level:u3}] {ThreadId} {Message:lj}{NewLine}{Exception}",
                    fileSizeLimitBytes: 10485760, // 10MB max file size
                    retainedFileCountLimit: 31 // Keep a month of logs
                )
                .CreateLogger();

            Log.Information("Application starting up");
        }

        private void ConfigureServices(IServiceCollection services)
        {
            services.AddSingleton<NetworkService>();
            services.AddSingleton<MainWindowViewModel>();
            services.AddTransient<MainWindow>();
        }

        private void Application_Startup(object sender, StartupEventArgs e)
        {
            Log.Information("Starting main window");
            
            var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
            mainWindow.DataContext = _serviceProvider.GetRequiredService<MainWindowViewModel>();
            mainWindow.Show();
            
            Log.Information("Main window started");
        }

        protected override void OnExit(ExitEventArgs e)
        {
            Log.Information("Application shutting down");
            _serviceProvider?.Dispose();
            Log.CloseAndFlush();
            base.OnExit(e);
        }
    }
}

