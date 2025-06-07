#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bank_logic.h"
//#include <winsock2.h>
//#include <ws2tcpip.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 2048
#define RESPONSE_SIZE 2048
char response[RESPONSE_SIZE];

void handle_client(const char *message, struct sockaddr_in *client_addr, socklen_t client_len, int sockfd);

int main(){
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    // Reap zombie processes automatically
    signal(SIGCHLD, SIG_IGN);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind to local address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Banking Server running on port %d...\n", SERVER_PORT);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0';

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            continue;
        }

        if (pid == 0) {
            handle_client(buffer, &client_addr, client_len, sockfd);
            exit(0);
        }
    }

    close(sockfd);
    return 0;
}

void handle_client(const char *message, struct sockaddr_in *client_addr, socklen_t client_len, int sockfd) {
    char response[RESPONSE_SIZE];

    // Log client info
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr->sin_port);
    printf("[INFO] Received request from %s:%d\n", client_ip, client_port);
    printf("[DEBUG] Raw message: %s\n", message);

    // Process request using new handler
    const char *result = process_request_extended(message);
    snprintf(response, sizeof(response), "%s", result);

    sendto(sockfd, response, strlen(response), 0,
           (struct sockaddr *)client_addr, client_len);
}
