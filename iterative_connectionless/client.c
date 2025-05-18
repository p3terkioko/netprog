#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include "bank_utils.h"

// Constants
#define MAX_MSG_LEN 1024
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

// Function prototypes
void clear_screen();
void display_menu();
void handle_user_input();
void clear_input_buffer();
void wait_for_enter();
void initialize_client(const char *ip, int port);

// Global socket variables
int sockfd;
struct sockaddr_in server_addr;

void initialize_client(const char *ip, int port)
{
    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
}

char *send_request(const char *request)
{
    char *response = malloc(MAX_MSG_LEN);
    if (!response)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Send request to server
    if (sendto(sockfd, request, strlen(request), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Send failed");
        free(response);
        return NULL;
    }

    // Receive response from server
    if (recvfrom(sockfd, response, MAX_MSG_LEN, 0, NULL, NULL) < 0)
    {
        perror("Receive failed");
        free(response);
        return NULL;
    }

    return response;
}

void clear_screen()
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

void display_menu()
{
    clear_screen();
    printf("\nBANKING MENU\n");
    printf("============\n");
    printf("1. Register New Account\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Check Balance\n");
    printf("5. Get Statement\n");
    printf("6. Close Account\n");
    printf("0. Exit\n");
    printf("\nEnter choice: ");
}

void wait_for_enter()
{
    printf("\nPress ENTER to return to the main menu...");
    while (getchar() != '\n')
        ;
}

void handle_user_input()
{
    int choice;
    char request[MAX_MSG_LEN];
    char account_no[MAX_ACCT_LEN], amount_str[MAX_AMT_LEN], pin[8];
    char name[64], national_id[32], account_type[16];
    double amount;

    while (1)
    {
        display_menu();
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (choice == 0)
        {
            printf("Thank you for using our banking service. Goodbye!\n");
            break;
        }

        switch (choice)
        {
        case 1:
        { // Register New Account
            clear_screen();
            printf("=== ACCOUNT REGISTRATION ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Name: ");
            if (!fgets(name, sizeof(name), stdin))
                break;
            trim(name);
            if (strcmp(name, "b") == 0)
                break;

            printf("Enter National ID: ");
            if (!fgets(national_id, sizeof(national_id), stdin))
                break;
            trim(national_id);
            if (strcmp(national_id, "b") == 0)
                break;

            printf("Enter Account Type (savings/checking): ");
            if (!fgets(account_type, sizeof(account_type), stdin))
                break;
            trim(account_type);
            if (strcmp(account_type, "b") == 0)
                break;

            printf("Enter Initial Deposit (minimum 1000): ");
            if (!fgets(amount_str, MAX_AMT_LEN, stdin))
                break;
            trim(amount_str);
            if (strcmp(amount_str, "b") == 0)
                break;
            amount = atof(amount_str);

            snprintf(request, MAX_MSG_LEN, "REGISTER %s %s %s 0000 %.2f",
                     name, national_id, account_type, amount);
            break;
        }

        case 2:
        { // Deposit
            clear_screen();
            printf("=== DEPOSIT ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Account Number: ");
            if (!fgets(account_no, MAX_ACCT_LEN, stdin))
                break;
            trim(account_no);
            if (strcmp(account_no, "b") == 0)
                break;

            printf("Enter PIN: ");
            if (!fgets(pin, sizeof(pin), stdin))
                break;
            trim(pin);
            if (strcmp(pin, "b") == 0)
                break;

            printf("Enter Amount (minimum 500): ");
            if (!fgets(amount_str, MAX_AMT_LEN, stdin))
                break;
            trim(amount_str);
            if (strcmp(amount_str, "b") == 0)
                break;
            amount = atof(amount_str);

            snprintf(request, MAX_MSG_LEN, "DEPOSIT %s %s %.2f",
                     account_no, pin, amount);
            break;
        }

        case 3:
        { // Withdraw
            clear_screen();
            printf("=== WITHDRAW ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Account Number: ");
            if (!fgets(account_no, MAX_ACCT_LEN, stdin))
                break;
            trim(account_no);
            if (strcmp(account_no, "b") == 0)
                break;

            printf("Enter PIN: ");
            if (!fgets(pin, sizeof(pin), stdin))
                break;
            trim(pin);
            if (strcmp(pin, "b") == 0)
                break;

            printf("Enter Amount (minimum 500): ");
            if (!fgets(amount_str, MAX_AMT_LEN, stdin))
                break;
            trim(amount_str);
            if (strcmp(amount_str, "b") == 0)
                break;
            amount = atof(amount_str);

            snprintf(request, MAX_MSG_LEN, "WITHDRAW %s %s %.2f",
                     account_no, pin, amount);
            break;
        }

        case 4:
        { // Check Balance
            clear_screen();
            printf("=== CHECK BALANCE ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Account Number: ");
            if (!fgets(account_no, MAX_ACCT_LEN, stdin))
                break;
            trim(account_no);
            if (strcmp(account_no, "b") == 0)
                break;

            printf("Enter PIN: ");
            if (!fgets(pin, sizeof(pin), stdin))
                break;
            trim(pin);
            if (strcmp(pin, "b") == 0)
                break;

            snprintf(request, MAX_MSG_LEN, "CHECK_BALANCE %s %s 0",
                     account_no, pin);
            break;
        }

        case 5:
        { // Get Statement
            clear_screen();
            printf("=== GET STATEMENT ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Account Number: ");
            if (!fgets(account_no, MAX_ACCT_LEN, stdin))
                break;
            trim(account_no);
            if (strcmp(account_no, "b") == 0)
                break;

            printf("Enter PIN: ");
            if (!fgets(pin, sizeof(pin), stdin))
                break;
            trim(pin);
            if (strcmp(pin, "b") == 0)
                break;

            snprintf(request, MAX_MSG_LEN, "GET_STATEMENT %s %s 0",
                     account_no, pin);
            break;
        }

        case 6:
        { // Close Account
            clear_screen();
            printf("=== CLOSE ACCOUNT ===\n");
            printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

            printf("Enter Account Number: ");
            if (!fgets(account_no, MAX_ACCT_LEN, stdin))
                break;
            trim(account_no);
            if (strcmp(account_no, "b") == 0)
                break;

            printf("Enter PIN: ");
            if (!fgets(pin, sizeof(pin), stdin))
                break;
            trim(pin);
            if (strcmp(pin, "b") == 0)
                break;

            snprintf(request, MAX_MSG_LEN, "CLOSE_ACCOUNT %s %s 0",
                     account_no, pin);
            break;
        }

        default:
            printf("Invalid choice. Please try again.\n");
            wait_for_enter();
            continue;
        }

        // Send request to server and get response
        char *response = send_request(request);
        if (response)
        {
            char status[32];
            char message[MAX_MSG_LEN];
            if (sscanf(response, "%s %[^\n]", status, message) == 2)
            {
                if (strcmp(status, "OK") == 0)
                {
                    printf("\n%s\n", message);
                }
                else
                {
                    printf("\nError: %s\n", message);
                }
            }
            else
            {
                printf("\nError: Invalid response from server\n");
            }
            free(response);
        }
        else
        {
            printf("\nError: Failed to communicate with server\n");
        }
        wait_for_enter();
    }
}

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Banking Client - UDP Version\n");
    printf("===========================\n\n");

    initialize_client(argv[1], atoi(argv[2]));
    handle_user_input();

    close(sockfd);
    return 0;
}