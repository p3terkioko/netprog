#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "bank_utils.h"

// Constants
#define SERVER_PORT 8080
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

// Message operation codes
#define OP_REGISTER "REGISTER"
#define OP_DEPOSIT "DEPOSIT"
#define OP_WITHDRAW "WITHDRAW"
#define OP_CHECK "CHECK_BALANCE"
#define OP_STATEMENT "GET_STATEMENT"
#define OP_CLOSE "CLOSE_ACCOUNT"

// Response codes
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_ACCT_EXISTS "ACCOUNT_EXISTS"
#define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
#define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
#define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
#define RESP_INVALID_REQUEST "INVALID_REQUEST"

// Message handling functions
char *create_response(const char *status, const char *message)
{
    char *response = malloc(MAX_MSG_LEN);
    if (!response)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    snprintf(response, MAX_MSG_LEN, "%s %s", status, message);
    return response;
}

char *process_request(const char *request)
{
    char operation[MAX_LINE_LEN];
    char account_no[MAX_ACCT_LEN];
    char pin[8];
    double amount = 0;
    char name[64], national_id[32], account_type[16];
    char response_msg[MAX_MSG_LEN];

    // Parse the operation type
    if (sscanf(request, "%s", operation) != 1)
    {
        return create_response(RESP_INVALID_REQUEST, "Invalid request format");
    }

    if (strcmp(operation, OP_REGISTER) == 0)
    {
        if (sscanf(request, "%s %s %s %s %s %lf",
                   operation, name, national_id, account_type, pin, &amount) != 6)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid registration format");
        }

        if (amount < 1000)
        {
            return create_response(RESP_INVALID_AMOUNT, "Initial deposit must be at least 1000");
        }

        char acct_no[MAX_ACCT_LEN];
        if (!open_account(name, national_id, account_type, acct_no, pin, amount))
        {
            return create_response(RESP_ERROR, "Account registration failed");
        }

        snprintf(response_msg, MAX_MSG_LEN, "Account created! Number: %s, PIN: %s", acct_no, pin);
        return create_response(RESP_OK, response_msg);
    }
    else if (strcmp(operation, OP_DEPOSIT) == 0)
    {
        if (sscanf(request, "%s %s %s %lf", operation, account_no, pin, &amount) != 4)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid deposit format");
        }

        if (!is_valid_account_no(account_no))
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid account number format");
        }

        if (amount < 500)
        {
            return create_response(RESP_INVALID_AMOUNT, "Minimum deposit is 500");
        }

        if (!deposit_extended(account_no, pin, amount))
        {
            return create_response(RESP_ERROR, "Deposit failed. Check account number or PIN");
        }

        return create_response(RESP_OK, "Deposit successful");
    }
    else if (strcmp(operation, OP_WITHDRAW) == 0)
    {
        if (sscanf(request, "%s %s %s %lf", operation, account_no, pin, &amount) != 4)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid withdrawal format");
        }

        if (!is_valid_account_no(account_no))
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid account number format");
        }

        if (amount < 500)
        {
            return create_response(RESP_INVALID_AMOUNT, "Minimum withdrawal is 500");
        }

        if (!withdraw_extended(account_no, pin, amount))
        {
            return create_response(RESP_ERROR, "Withdrawal failed. Check account number, PIN, or balance");
        }

        return create_response(RESP_OK, "Withdrawal successful");
    }
    else if (strcmp(operation, OP_CHECK) == 0)
    {
        if (sscanf(request, "%s %s %s", operation, account_no, pin) != 3)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid balance check format");
        }

        if (!is_valid_account_no(account_no))
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid account number format");
        }

        double balance;
        if (!check_balance_extended(account_no, pin, &balance))
        {
            return create_response(RESP_ERROR, "Failed to check balance. Check account number or PIN");
        }

        snprintf(response_msg, MAX_MSG_LEN, "Current Balance: %.2f", balance);
        return create_response(RESP_OK, response_msg);
    }
    else if (strcmp(operation, OP_STATEMENT) == 0)
    {
        if (sscanf(request, "%s %s %s", operation, account_no, pin) != 3)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid statement request format");
        }

        if (!is_valid_account_no(account_no))
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid account number format");
        }

        char statement[1024];
        if (!get_statement_extended(account_no, pin, statement, sizeof(statement)))
        {
            return create_response(RESP_ERROR, "Failed to get statement. Check account number or PIN");
        }

        return create_response(RESP_OK, statement);
    }
    else if (strcmp(operation, OP_CLOSE) == 0)
    {
        if (sscanf(request, "%s %s %s", operation, account_no, pin) != 3)
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid close account format");
        }

        if (!is_valid_account_no(account_no))
        {
            return create_response(RESP_INVALID_REQUEST, "Invalid account number format");
        }

        double final_balance;
        if (!close_account_extended(account_no, pin, &final_balance))
        {
            return create_response(RESP_ERROR, "Failed to close account. Check account number or PIN");
        }

        snprintf(response_msg, MAX_MSG_LEN, "Account closed successfully. Final balance: %.2f", final_balance);
        return create_response(RESP_OK, response_msg);
    }
    else
    {
        return create_response(RESP_INVALID_REQUEST, "Unknown operation");
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_MSG_LEN];
    socklen_t client_len = sizeof(client_addr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind socket to server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP Server started on port %d\n", SERVER_PORT);

    while (1)
    {
        // Receive message from client
        memset(buffer, 0, MAX_MSG_LEN);
        if (recvfrom(sockfd, buffer, MAX_MSG_LEN, 0,
                     (struct sockaddr *)&client_addr, &client_len) < 0)
        {
            perror("Receive failed");
            continue;
        }

        printf("Received request from %s:%d: %s\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               buffer);

        // Process request
        char *response = process_request(buffer);

        // Send response back to client
        if (sendto(sockfd, response, strlen(response), 0,
                   (struct sockaddr *)&client_addr, client_len) < 0)
        {
            perror("Send failed");
        }

        free(response);
    }

    close(sockfd);
    return 0;
}