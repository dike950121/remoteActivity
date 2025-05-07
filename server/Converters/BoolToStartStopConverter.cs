using System;
using System.Globalization;
using System.Windows.Data;

namespace RemoteServer.Converters
{
    public class BoolToStartStopConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool isRunning && isRunning ? "Stop Server" : "Start Server";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}