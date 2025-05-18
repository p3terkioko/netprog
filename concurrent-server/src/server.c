#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include "bankv1.c"

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 9000

void handle_client_request(int client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_sock);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Parse the command and call the appropriate function
    char command[BUFFER_SIZE];
    sscanf(buffer, "%s", command);

    if (strcmp(command, "REGISTER") == 0) {
        char account[BUFFER_SIZE];
        sscanf(buffer, "%*s %s", account);
        register_account(account);
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "OK 0.00\n");
        send(client_sock, response, strlen(response), 0);
    } else if (strcmp(command, "DEPOSIT") == 0) {
        char account[BUFFER_SIZE];
        double amount;
        sscanf(buffer, "%*s %s %lf", account, &amount);
        deposit(account, amount);
        double balance = check_balance(account);
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "OK %.2f\n", balance);
        send(client_sock, response, strlen(response), 0);
    } else if (strcmp(command, "WITHDRAW") == 0) {
        char account[BUFFER_SIZE];
        double amount;
        sscanf(buffer, "%*s %s %lf", account, &amount);
        withdraw(account, amount);
        double balance = check_balance(account);
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "OK %.2f\n", balance);
        send(client_sock, response, strlen(response), 0);
    } else if (strcmp(command, "CHECK_BALANCE") == 0) {
        char account[BUFFER_SIZE];
        sscanf(buffer, "%*s %s", account);
        double balance = check_balance(account);
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "OK %.2f\n", balance);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "ERROR Unknown command\n");
        send(client_sock, response, strlen(response), 0);
    }

    close(client_sock);
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    int server_port = DEFAULT_PORT;
    if (argc == 2) {
        server_port = atoi(argv[1]);
    }

    int listenfd, connfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    signal(SIGCHLD, sigchld_handler);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(server_port);

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", server_port);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
        if (connfd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) { // Child
            close(listenfd);
            handle_client_request(connfd);
            exit(0);
        } else if (pid > 0) { // Parent
            close(connfd);
        } else {
            perror("fork");
            close(connfd);
        }
    }

    close(listenfd);
    return 0;
}