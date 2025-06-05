#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "bank_logic.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #undef fileno
    #define fileno _fileno
#else
    #include <unistd.h>
    #include <sys/file.h>
#endif

/*
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif*/

#define PORT 8080
#define BACKLOG 5

void handle_client(int client_sock) {
    char buffer[MAX_MSG_LEN];
    long bytes_received;

    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate the received string

        // Process the request using bank logic
        char* response = process_request(buffer);

        // Send the response back to the client
        if (send(client_sock, response, strlen(response), 0) == -1) {
            perror("send");
        }

        free(response);  // response was dynamically allocated
    }

    if (bytes_received == -1) {
        perror("recv");
    }

    close(client_sock);
    printf("Client disconnected.\n");
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_sock, BACKLOG) == -1) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Main loop to accept connections
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

        printf("Accepted a connection.\n");
        handle_client(client_sock);  // Iterative handling: one at a time
    }

    close(server_sock);
    WSACleanup();
    return 0;
}
