// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bank_logic.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Define your server info
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

extern void handle_user_input(int sockfd);  // from client_ui.c

int connect_to_server() {
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return -1;
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        int err = WSAGetLastError();
        printf("Socket creation failed: %d\n", err);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IP from text to binary
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (serv_addr.sin_addr.s_addr == INADDR_NONE){
        printf("Invalid IP address format\n");
        closesocket(sockfd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        printf("Connect failed: %d\n", err);
        closesocket(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);
    return sockfd;
}

int main() {
    int sockfd = connect_to_server();

    // Call the UI that sends/receives requests through this socket
    handle_user_input(sockfd);

    // Close socket after done
    closesocket(sockfd);
    WSACleanup();
    printf("Disconnected from server. Exiting.\n");
    return 0;
}
