#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define DATA_FILE "data.txt"
#define BUFFER_SIZE 1024

void register_account(const char *account) {
    // Implementation for registering an account
}

void deposit(const char *account, double amount) {
    // Implementation for depositing money
}

void withdraw(const char *account, double amount) {
    // Implementation for withdrawing money
}

double check_balance(const char *account) {
    // Implementation for checking balance
    return 0.0; // Placeholder return value
}

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
    } else if (strcmp(command, "DEPOSIT") == 0) {
        char account[BUFFER_SIZE];
        double amount;
        sscanf(buffer, "%*s %s %lf", account, &amount);
        deposit(account, amount);
    } else if (strcmp(command, "WITHDRAW") == 0) {
        char account[BUFFER_SIZE];
        double amount;
        sscanf(buffer, "%*s %s %lf", account, &amount);
        withdraw(account, amount);
    } else if (strcmp(command, "CHECK_BALANCE") == 0) {
        char account[BUFFER_SIZE];
        sscanf(buffer, "%*s %s", account);
        double balance = check_balance(account);
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "Balance for %s: %.2f", account, balance);
        send(client_sock, response, strlen(response), 0);
    }

    close(client_sock);
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    // Implementation for the server setup and accepting connections
    return 0;
}