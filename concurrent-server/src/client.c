#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"
#include "utils.h"

#define SERVER_PORT 9000
#define BUFFER_SIZE 1024

void display_menu() {
    printf("Menu:\n");
    printf("1. Register\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Check Balance\n");
    printf("5. Exit\n");
}

void send_request(int sockfd, const char *request) {
    send(sockfd, request, strlen(request), 0);
}

void handle_server_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Error receiving data from server");
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string
    printf("Server response: %s\n", buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int choice;
    char request[BUFFER_SIZE];

    while (1) {
        display_menu();
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Consume newline

        switch (choice) {
            case 1:
                printf("Enter account name to register: ");
                fgets(request, sizeof(request), stdin);
                request[strcspn(request, "\n")] = 0; // Remove newline
                snprintf(request, sizeof(request), "REGISTER %s", request);
                break;
            case 2:
                printf("Enter account name and amount to deposit (e.g., acct 100): ");
                fgets(request, sizeof(request), stdin);
                request[strcspn(request, "\n")] = 0; // Remove newline
                snprintf(request, sizeof(request), "DEPOSIT %s", request);
                break;
            case 3:
                printf("Enter account name and amount to withdraw (e.g., acct 50): ");
                fgets(request, sizeof(request), stdin);
                request[strcspn(request, "\n")] = 0; // Remove newline
                snprintf(request, sizeof(request), "WITHDRAW %s", request);
                break;
            case 4:
                printf("Enter account name to check balance: ");
                fgets(request, sizeof(request), stdin);
                request[strcspn(request, "\n")] = 0; // Remove newline
                snprintf(request, sizeof(request), "CHECK_BALANCE %s", request);
                break;
            case 5:
                snprintf(request, sizeof(request), "EXIT");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        send_request(sockfd, request);
        if (choice == 5) {
            break;
        }

        handle_server_response(sockfd);
    }

    // Close the socket
    close(sockfd);
    return 0;
}