#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include "../bank_utils.h"

// Constants
#define SERVER_PORT 8080
#define MAX_MSG_LEN 256
#define MAX_CLIENTS 5
#define MAX_STATEMENT_LEN 1024

// Function prototypes
void handle_client(int client_socket);
void clear_screen();
void display_menu();
void handle_user_input();
void clear_input_buffer();
void wait_for_enter();
char *create_message(const char *operation, const char *account_no, double amount);
bool parse_response(const char *response, char *status, double *balance);

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    while (1)
    {
        // Accept client connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("New client connected from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Handle client in the same process
        handle_client(client_socket);

        // Close client socket
        close(client_socket);
    }

    // Close server socket
    close(server_socket);
    return 0;
}

void handle_client(int client_socket)
{
    char buffer[MAX_MSG_LEN];
    char response[MAX_MSG_LEN];
    char operation[32], account_no[MAX_ACCT_LEN], pin[8];
    double amount;
    int bytes_received;

    while ((bytes_received = recv(client_socket, buffer, MAX_MSG_LEN - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("Received: %s\n", buffer);

        // Parse the request
        if (sscanf(buffer, "%s %s %s %lf", operation, account_no, pin, &amount) >= 3)
        {
            if (strcmp(operation, "REGISTER") == 0)
            {
                char name[64], national_id[32], account_type[16], out_account_no[MAX_ACCT_LEN], out_pin[8];
                if (sscanf(buffer, "%*s %*s %*s %*s %s %s %s %lf", name, national_id, account_type, &amount) == 4)
                {
                    if (open_account(name, national_id, account_type, out_account_no, out_pin, amount))
                    {
                        snprintf(response, MAX_MSG_LEN, "OK %s %s", out_account_no, out_pin);
                    }
                    else
                    {
                        strcpy(response, "ERROR");
                    }
                }
                else
                {
                    strcpy(response, "INVALID_REQUEST");
                }
            }
            else if (strcmp(operation, "DEPOSIT") == 0)
            {
                if (deposit_extended(account_no, pin, amount))
                {
                    strcpy(response, "OK");
                }
                else
                {
                    strcpy(response, "ERROR");
                }
            }
            else if (strcmp(operation, "WITHDRAW") == 0)
            {
                if (withdraw_extended(account_no, pin, amount))
                {
                    strcpy(response, "OK");
                }
                else
                {
                    strcpy(response, "ERROR");
                }
            }
            else if (strcmp(operation, "CHECK_BALANCE") == 0)
            {
                double balance;
                if (check_balance_extended(account_no, pin, &balance))
                {
                    snprintf(response, MAX_MSG_LEN, "OK %.2f", balance);
                }
                else
                {
                    strcpy(response, "ERROR");
                }
            }
            else if (strcmp(operation, "GET_STATEMENT") == 0)
            {
                char statement[MAX_STATEMENT_LEN];
                if (get_statement_extended(account_no, pin, statement, sizeof(statement)))
                {
                    // Calculate available space in response buffer
                    size_t available_space = MAX_MSG_LEN - 4; // Reserve space for "OK "
                    size_t statement_len = strlen(statement);

                    // Truncate statement if it's too long
                    if (statement_len > available_space)
                    {
                        statement[available_space] = '\0';
                    }

                    // Use a more precise format string with length limit
                    int written = snprintf(response, MAX_MSG_LEN, "OK %.*s",
                                           (int)available_space, statement);
                    if (written < 0 || (size_t)written >= MAX_MSG_LEN)
                    {
                        // If truncation occurred, ensure null termination
                        response[MAX_MSG_LEN - 1] = '\0';
                    }
                }
                else
                {
                    strcpy(response, "ERROR");
                }
            }
            else if (strcmp(operation, "CLOSE_ACCOUNT") == 0)
            {
                double final_balance;
                if (close_account_extended(account_no, pin, &final_balance))
                {
                    snprintf(response, MAX_MSG_LEN, "OK %.2f", final_balance);
                }
                else
                {
                    strcpy(response, "ERROR");
                }
            }
            else
            {
                strcpy(response, "INVALID_REQUEST");
            }
        }
        else
        {
            strcpy(response, "INVALID_REQUEST");
        }

        // Send response
        if (send(client_socket, response, strlen(response), 0) < 0)
        {
            perror("Send failed");
            break;
        }
    }

    if (bytes_received < 0)
    {
        perror("Receive failed");
    }
}