using System;
using System.IO;
using System.Threading;

namespace RemoteAccessServer.Core
{
    /// <summary>
    /// Centralized logging system for the Remote Access Server
    /// </summary>
    public static class Logger
    {
        private static readonly object _lockObject = new object();
        private static string _logFilePath;
        private static bool _isInitialized = false;

        /// <summary>
        /// Initialize the logging system
        /// </summary>
        public static void Initialize()
        {
            if (_isInitialized) return;

            try
            {
                var logDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Logs");
                if (!Directory.Exists(logDirectory))
                {
                    Directory.CreateDirectory(logDirectory);
                }

                var timestamp = DateTime.Now.ToString("yyyy-MM-dd");
                _logFilePath = Path.Combine(logDirectory, $"server_{timestamp}.log");
                
                _isInitialized = true;
                Log("Logger initialized successfully");
            }
            catch (Exception ex)
            {
                // If we can't initialize logging, we'll just continue without it
                Console.WriteLine($"Failed to initialize logger: {ex.Message}");
            }
        }

        /// <summary>
        /// Log an informational message
        /// </summary>
        /// <param name="message">The message to log</param>
        public static void Log(string message)
        {
            WriteLog("INFO", message);
        }

        /// <summary>
        /// Log an error message
        /// </summary>
        /// <param name="message">The error message to log</param>
        public static void LogError(string message)
        {
            WriteLog("ERROR", message);
        }

        /// <summary>
        /// Log a warning message
        /// </summary>
        /// <param name="message">The warning message to log</param>
        public static void LogWarning(string message)
        {
            WriteLog("WARNING", message);
        }

        /// <summary>
        /// Log a debug message
        /// </summary>
        /// <param name="message">The debug message to log</param>
        public static void LogDebug(string message)
        {
#if DEBUG
            WriteLog("DEBUG", message);
#endif
        }

        /// <summary>
        /// Write a log entry with the specified level
        /// </summary>
        /// <param name="level">The log level</param>
        /// <param name="message">The message to log</param>
        private static void WriteLog(string level, string message)
        {
            if (!_isInitialized) return;

            try
            {
                lock (_lockObject)
                {
                    var timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff");
                    var threadId = Thread.CurrentThread.ManagedThreadId;
                    var logEntry = $"[{timestamp}] [{level}] [Thread-{threadId}] {message}";

                    // Write to file
                    File.AppendAllText(_logFilePath, logEntry + Environment.NewLine);

                    // Also write to console in debug mode
#if DEBUG
                    Console.WriteLine(logEntry);
#endif
                }
            }
            catch (Exception ex)
            {
                // If logging fails, write to console as fallback
                Console.WriteLine($"Logging failed: {ex.Message}");
                Console.WriteLine($"Original message: [{level}] {message}");
            }
        }

        /// <summary>
        /// Log an exception with full details
        /// </summary>
        /// <param name="ex">The exception to log</param>
        /// <param name="context">Additional context information</param>
        public static void LogException(Exception ex, string context = "")
        {
            var message = context != null ? $"{context}: {ex}" : ex.ToString();
            LogError(message);
        }
    }
}