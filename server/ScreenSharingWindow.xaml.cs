using System;
using System.Windows;
using System.Windows.Media.Imaging;
using System.IO;
using System.Windows.Threading;
using Serilog;

namespace RemoteServer
{
    public partial class ScreenSharingWindow : Window
    {
        private int _imageWidth;
        private int _imageHeight;
        private long _imageSize;
        private byte[]? _imageBuffer;
        private int _bytesReceived;
        private bool _receivingImage;

        public ScreenSharingWindow()
        {
            InitializeComponent();
            _receivingImage = false;
            Title = "Screen Sharing - Waiting for image...";
        }

        public void ProcessImageHeader(string header)
        {
            try
            {
                // Parse header: SCREEN_IMAGE:width:height:size
                var parts = header.Split(':');
                if (parts.Length != 4 || parts[0] != "SCREEN_IMAGE")
                {
                    Log.Error("Invalid image header format: {Header}", header);
                    return;
                }

                _imageWidth = int.Parse(parts[1]);
                _imageHeight = int.Parse(parts[2]);
                _imageSize = long.Parse(parts[3]);
                
                // Allocate buffer for the image data
                _imageBuffer = new byte[_imageSize];
                _bytesReceived = 0;
                _receivingImage = true;

                Log.Information("Receiving image: {Width}x{Height}, {Size} bytes", _imageWidth, _imageHeight, _imageSize);
                
                Dispatcher.Invoke(() => {
                    Title = $"Screen Sharing - Receiving {_imageWidth}x{_imageHeight} ({_imageSize} bytes)...";
                });
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error processing image header: {Header}", header);
            }
        }

        public void ProcessImageData(byte[] data, int length)
        {
            if (!_receivingImage || _imageBuffer == null)
            {
                Log.Warning("Received image data but not expecting an image");
                return;
            }

            try
            {
                // Copy the received chunk to our buffer
                if (_bytesReceived + length <= _imageSize)
                {
                    Buffer.BlockCopy(data, 0, _imageBuffer, _bytesReceived, length);
                    _bytesReceived += length;

                    // Update progress in title
                    Dispatcher.Invoke(() => {
                        Title = $"Screen Sharing - Receiving: {_bytesReceived * 100 / _imageSize}%";
                    });

                    // Check if we've received the complete image
                    if (_bytesReceived == _imageSize)
                    {
                        DisplayImage();
                    }
                }
                else
                {
                    Log.Error("Received more image data than expected");
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error processing image data chunk");
            }
        }

        private void DisplayImage()
        {
            if (_imageBuffer == null)
            {
                Log.Error("Cannot display image: Image buffer is null");
                return;
            }

            try
            {
                // Create bitmap header
                var fileHeaderSize = 14;
                var infoHeaderSize = 40;
                var fileSize = fileHeaderSize + infoHeaderSize + _imageSize;
                
                var bmpFileHeader = new byte[fileHeaderSize];
                var bmpInfoHeader = new byte[infoHeaderSize];
                
                // File header
                bmpFileHeader[0] = (byte)'B';
                bmpFileHeader[1] = (byte)'M';
                bmpFileHeader[2] = (byte)(fileSize);
                bmpFileHeader[3] = (byte)(fileSize >> 8);
                bmpFileHeader[4] = (byte)(fileSize >> 16);
                bmpFileHeader[5] = (byte)(fileSize >> 24);
                bmpFileHeader[10] = (byte)(fileHeaderSize + infoHeaderSize);
                
                // Info header
                bmpInfoHeader[0] = (byte)(infoHeaderSize);
                bmpInfoHeader[4] = (byte)(_imageWidth);
                bmpInfoHeader[5] = (byte)(_imageWidth >> 8);
                bmpInfoHeader[6] = (byte)(_imageWidth >> 16);
                bmpInfoHeader[7] = (byte)(_imageWidth >> 24);
                bmpInfoHeader[8] = (byte)(_imageHeight);
                bmpInfoHeader[9] = (byte)(_imageHeight >> 8);
                bmpInfoHeader[10] = (byte)(_imageHeight >> 16);
                bmpInfoHeader[11] = (byte)(_imageHeight >> 24);
                bmpInfoHeader[12] = (byte)(1);
                bmpInfoHeader[14] = (byte)(24); // 24 bits per pixel
                
                // Create a memory stream with the complete BMP file
                using (var ms = new MemoryStream())
                {
                    ms.Write(bmpFileHeader, 0, fileHeaderSize);
                    ms.Write(bmpInfoHeader, 0, infoHeaderSize);
                    ms.Write(_imageBuffer, 0, _imageBuffer.Length);
                    ms.Seek(0, SeekOrigin.Begin);
                    
                    // Create bitmap from the memory stream
                    var bitmap = new BitmapImage();
                    bitmap.BeginInit();
                    bitmap.CacheOption = BitmapCacheOption.OnLoad;
                    bitmap.StreamSource = ms;
                    bitmap.EndInit();
                    bitmap.Freeze(); // Make it thread-safe
                    
                    // Display the image
                    Dispatcher.Invoke(() => {
                        ScreenImage.Source = bitmap;
                        Title = $"Screen Sharing - {_imageWidth}x{_imageHeight}";
                    });
                    
                    Log.Information("Image displayed successfully");
                }
                
                // Reset for next image
                _receivingImage = false;
                _imageBuffer = null;
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Error displaying image");
                
                Dispatcher.Invoke(() => {
                    Title = "Screen Sharing - Error displaying image";
                });
            }
        }
    }
}
