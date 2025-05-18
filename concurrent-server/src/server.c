#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "common.h"
#include "utils.h"

#define SERVER_PORT 9000
#define BUFFER_SIZE 1024
#define DATA_FILE "data.txt"

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Error receiving data from client");
        close(client_sock);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Parse the command and call corresponding functions from bankv1.c
    // Example: "REGISTER <acct>", "DEPOSIT <acct> <amt>", etc.
    // Implement the logic to handle each command here

    // Send response back to client
    const char *response = "Command processed"; // Placeholder response
    send(client_sock, response, strlen(response), 0);
    close(client_sock);
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Handle SIGCHLD to clean up zombie processes
    signal(SIGCHLD, sigchld_handler);

    printf("Server listening on port %d...\n", SERVER_PORT);

    while (1) {
        // Accept a new client connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Fork a new process to handle the client
        if (fork() == 0) {
            close(server_sock); // Child process doesn't need the listener
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock); // Parent process doesn't need the client socket
    }

    close(server_sock);
    return 0;
}