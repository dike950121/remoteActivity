using System;
using System.Windows;
using Microsoft.Extensions.DependencyInjection;
using RemoteServer.Services;
using RemoteServer.ViewModels;
using Serilog;

namespace RemoteServer
{
    public partial class App : Application
    {
        private ServiceProvider _serviceProvider = null!;

        public App()
        {
            var services = new ServiceCollection();
            ConfigureServices(services);
            _serviceProvider = services.BuildServiceProvider();

            // Configure logging
            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Debug()
                .WriteTo.File("server_log.txt", rollingInterval: RollingInterval.Day)
                .CreateLogger();
        }

        private void ConfigureServices(IServiceCollection services)
        {
            services.AddSingleton<NetworkService>();
            services.AddSingleton<MainWindowViewModel>();
            services.AddTransient<MainWindow>();
        }

        private void Application_Startup(object sender, StartupEventArgs e)
        {
            var mainWindow = _serviceProvider.GetRequiredService<MainWindow>();
            mainWindow.DataContext = _serviceProvider.GetRequiredService<MainWindowViewModel>();
            mainWindow.Show();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            _serviceProvider?.Dispose();
            Log.CloseAndFlush();
            base.OnExit(e);
        }
    }
}

