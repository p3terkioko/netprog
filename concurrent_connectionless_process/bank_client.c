// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bank_logic.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

extern void handle_user_input(int sockfd, struct sockaddr_in *server_addr);  // from client_ui.c

int setup_udp_socket(struct sockaddr_in *server_addr) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr->sin_addr);

    return sockfd;
}

int main() {
    struct sockaddr_in server_addr;
    int sockfd = setup_udp_socket(&server_addr);

    printf("UDP client ready. Sending to %s:%d\n", SERVER_IP, SERVER_PORT);

    handle_user_input(sockfd, &server_addr);

    close(sockfd);
    printf("Client exiting.\n");
    return 0;
}
