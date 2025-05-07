#include "remote_client.h"
#include <windows.h>

// Function to capture the screen and send it to the server
RemoteClientError capture_and_send_screen() {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
    
    if (!hdcScreen || !hdcMemDC) {
        log_message(LOG_ERROR, "Failed to create device context for screen capture");
        if (hdcMemDC) DeleteDC(hdcMemDC);
        if (hdcScreen) ReleaseDC(NULL, hdcScreen);
        return RC_ERROR_SYSTEM;
    }
    
    // Get the screen dimensions
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    
    log_message(LOG_DEBUG, "Screen dimensions: %dx%d", width, height);
    
    // Create a bitmap compatible with the screen DC
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    if (!hBitmap) {
        log_message(LOG_ERROR, "Failed to create compatible bitmap");
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return RC_ERROR_SYSTEM;
    }
    
    // Select the bitmap into the memory DC
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);
    
    // Copy from the screen DC to the memory DC
    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY)) {
        log_message(LOG_ERROR, "BitBlt failed: %d", GetLastError());
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return RC_ERROR_SYSTEM;
    }
    
    // Get the bitmap information
    BITMAP bmpScreen;
    GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);
    
    // Create a BITMAPINFOHEADER structure for the bitmap
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24; // 24 bits per pixel (RGB)
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    
    // Calculate the size of the bitmap data
    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    
    // Allocate memory for the bitmap data
    BYTE* lpBitmap = (BYTE*)malloc(dwBmpSize);
    if (!lpBitmap) {
        log_message(LOG_ERROR, "Failed to allocate memory for bitmap data");
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return RC_ERROR_MEMORY;
    }
    
    // Get the bitmap data
    if (!GetDIBits(hdcScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpBitmap, 
                  (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        log_message(LOG_ERROR, "GetDIBits failed: %d", GetLastError());
        free(lpBitmap);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return RC_ERROR_SYSTEM;
    }
    
    // First, send a header message with image dimensions
    Message header_msg = { .type = MSG_TYPE_RESPONSE };
    char header_text[100];
    snprintf(header_text, sizeof(header_text), 
             "SCREEN_IMAGE:%d:%d:%lu", 
             width, height, dwBmpSize);
    
    header_msg.data_length = strlen(header_text);
    memcpy(header_msg.data, header_text, header_msg.data_length);
    
    // Send the header
    RemoteClientError result = send_message(&header_msg);
    if (result != RC_SUCCESS) {
        log_message(LOG_ERROR, "Failed to send image header");
        free(lpBitmap);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return result;
    }
    
    // Now send the image data in chunks
    size_t bytes_sent = 0;
    size_t chunk_size = MAX_MESSAGE_SIZE;
    Message data_msg = { .type = MSG_TYPE_RESPONSE };
    
    while (bytes_sent < dwBmpSize) {
        size_t remaining = dwBmpSize - bytes_sent;
        size_t current_chunk = remaining < chunk_size ? remaining : chunk_size;
        
        data_msg.data_length = current_chunk;
        memcpy(data_msg.data, lpBitmap + bytes_sent, current_chunk);
        
        log_message(LOG_DEBUG, "Sending image chunk: %zu/%lu bytes", bytes_sent + current_chunk, dwBmpSize);
        
        result = send_message(&data_msg);
        if (result != RC_SUCCESS) {
            log_message(LOG_ERROR, "Failed to send image data chunk at offset %zu", bytes_sent);
            free(lpBitmap);
            SelectObject(hdcMemDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMemDC);
            ReleaseDC(NULL, hdcScreen);
            return result;
        }
        
        bytes_sent += current_chunk;
    }
    
    // Clean up
    free(lpBitmap);
    SelectObject(hdcMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    
    log_message(LOG_INFO, "Screen capture completed and sent: %dx%d (%lu bytes)", width, height, dwBmpSize);
    return RC_SUCCESS;
} 