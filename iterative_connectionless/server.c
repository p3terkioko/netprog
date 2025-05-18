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

// Message handling functions
char *create_response(const char *status, double balance)
{
    char *response = malloc(MAX_MSG_LEN);
    if (!response)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    snprintf(response, MAX_MSG_LEN, "%s %.2f", status, balance);
    return response;
}

bool parse_message(const char *message, char *operation, char *account_no, double *amount)
{
    int result = sscanf(message, "%s %s %lf", operation, account_no, amount);
    return (result >= 2); // At least operation and account_no were parsed
}

char *process_request(const char *request)
{
    char operation[MAX_LINE_LEN];
    char account_no[MAX_ACCT_LEN];
    double amount = 0;

    if (!parse_message(request, operation, account_no, &amount))
    {
        return create_response(RESP_INVALID_REQUEST, 0);
    }

    if (strcmp(operation, OP_REGISTER) == 0)
    {
        if (account_exists(account_no))
        {
            return create_response(RESP_ACCT_EXISTS, 0);
        }
        if (add_account(account_no, 0))
        {
            return create_response(RESP_OK, 0);
        }
        return create_response(RESP_ERROR, 0);
    }
    else if (strcmp(operation, OP_DEPOSIT) == 0)
    {
        double balance = 0;
        if (!is_valid_amount(amount))
        {
            return create_response(RESP_INVALID_AMOUNT, 0);
        }
        if (!account_exists(account_no))
        {
            return create_response(RESP_ACCT_NOT_FOUND, 0);
        }
        if (!get_balance(account_no, &balance))
        {
            return create_response(RESP_ERROR, 0);
        }
        balance += amount;
        if (update_balance(account_no, balance))
        {
            log_transaction(account_no, "DEPOSIT", amount, balance);
            return create_response(RESP_OK, balance);
        }
        return create_response(RESP_ERROR, 0);
    }
    else if (strcmp(operation, OP_WITHDRAW) == 0)
    {
        double balance = 0;
        if (!is_valid_amount(amount))
        {
            return create_response(RESP_INVALID_AMOUNT, 0);
        }
        if (!account_exists(account_no))
        {
            return create_response(RESP_ACCT_NOT_FOUND, 0);
        }
        if (!get_balance(account_no, &balance))
        {
            return create_response(RESP_ERROR, 0);
        }
        if (balance < amount)
        {
            return create_response(RESP_INSUFFICIENT_FUNDS, balance);
        }
        balance -= amount;
        if (update_balance(account_no, balance))
        {
            log_transaction(account_no, "WITHDRAW", amount, balance);
            return create_response(RESP_OK, balance);
        }
        return create_response(RESP_ERROR, 0);
    }
    else if (strcmp(operation, OP_CHECK) == 0)
    {
        double balance = 0;
        if (!account_exists(account_no))
        {
            return create_response(RESP_ACCT_NOT_FOUND, 0);
        }
        if (get_balance(account_no, &balance))
        {
            return create_response(RESP_OK, balance);
        }
        return create_response(RESP_ERROR, 0);
    }
    else
    {
        return create_response(RESP_INVALID_REQUEST, 0);
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