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
#define SERVER_IP "127.0.0.1" // localhost
#define SERVER_PORT 8080
#define MAX_MSG_LEN 256
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

// Function prototypes
void clear_screen();
void display_menu();
void handle_user_input();
void clear_input_buffer();
void wait_for_enter();
char *create_message(const char *operation, const char *account_no, double amount);
bool parse_response(const char *response, char *status, double *balance);
bool send_request(const char *request, char *response, size_t response_size);

// Global socket variables
int sockfd;
struct sockaddr_in server_addr;

void initialize_client()
{
    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
}

bool send_request(const char *request, char *response, size_t response_size)
{
    // Send request to server
    if (send(sockfd, request, strlen(request), 0) < 0)
    {
        perror("Send failed");
        return false;
    }

    // Receive response from server
    int bytes_received = recv(sockfd, response, response_size - 1, 0);
    if (bytes_received < 0)
    {
        perror("Receive failed");
        return false;
    }
    response[bytes_received] = '\0';
    return true;
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
    char account_no[MAX_ACCT_LEN], amount_str[MAX_AMT_LEN], pin[8];
    double amount;
    char request[MAX_MSG_LEN];
    char response[MAX_MSG_LEN];

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
            char name[64], national_id[32], account_type[16], acct_no[MAX_ACCT_LEN], pin[8], deposit_str[32];
            double initial_deposit = 0.0;

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
            if (!fgets(deposit_str, sizeof(deposit_str), stdin))
                break;
            trim(deposit_str);
            if (strcmp(deposit_str, "b") == 0)
                break;
            initial_deposit = atof(deposit_str);

            if (initial_deposit < 1000)
            {
                printf("Initial deposit must be at least 1000.\n");
            }
            else
            {
                snprintf(request, MAX_MSG_LEN, "REGISTER %s %s %s %s %.2f",
                         name, national_id, account_type, "0000", initial_deposit);
                if (send_request(request, response, MAX_MSG_LEN))
                {
                    char status[32];
                    if (sscanf(response, "%s %s %s", status, acct_no, pin) == 3 &&
                        strcmp(status, "OK") == 0)
                    {
                        printf("Account created! Number: %s, PIN: %s\n", acct_no, pin);
                    }
                    else
                    {
                        printf("Account registration failed.\n");
                    }
                }
            }
            wait_for_enter();
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

            if (!is_valid_account_no(account_no))
            {
                printf("Invalid account number format.\n");
                wait_for_enter();
                break;
            }

            printf("Enter Amount (minimum 500): ");
            if (!fgets(amount_str, MAX_AMT_LEN, stdin))
                break;
            trim(amount_str);
            if (strcmp(amount_str, "b") == 0)
                break;
            amount = atof(amount_str);

            if (amount < 500)
            {
                printf("Minimum deposit is 500.\n");
            }
            else
            {
                snprintf(request, MAX_MSG_LEN, "DEPOSIT %s %s %.2f", account_no, pin, amount);
                if (send_request(request, response, MAX_MSG_LEN))
                {
                    if (strcmp(response, "OK") == 0)
                    {
                        printf("Deposit successful.\n");
                    }
                    else
                    {
                        printf("Deposit failed. Check account number or PIN.\n");
                    }
                }
            }
            wait_for_enter();
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

            if (!is_valid_account_no(account_no))
            {
                printf("Invalid account number format.\n");
                wait_for_enter();
                break;
            }

            printf("Enter Amount (minimum 500, must leave at least 1000 in account): ");
            if (!fgets(amount_str, MAX_AMT_LEN, stdin))
                break;
            trim(amount_str);
            if (strcmp(amount_str, "b") == 0)
                break;
            amount = atof(amount_str);

            if (amount < 500)
            {
                printf("Minimum withdrawal is 500.\n");
            }
            else
            {
                snprintf(request, MAX_MSG_LEN, "WITHDRAW %s %s %.2f", account_no, pin, amount);
                if (send_request(request, response, MAX_MSG_LEN))
                {
                    if (strcmp(response, "OK") == 0)
                    {
                        printf("Withdrawal successful.\n");
                    }
                    else
                    {
                        printf("Withdrawal failed. Check account number, PIN, or balance.\n");
                    }
                }
            }
            wait_for_enter();
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

            if (!is_valid_account_no(account_no))
            {
                printf("Invalid account number format.\n");
                wait_for_enter();
                break;
            }

            snprintf(request, MAX_MSG_LEN, "CHECK_BALANCE %s %s 0", account_no, pin);
            if (send_request(request, response, MAX_MSG_LEN))
            {
                char status[32];
                double balance;
                if (sscanf(response, "%s %lf", status, &balance) == 2 &&
                    strcmp(status, "OK") == 0)
                {
                    printf("Current Balance: %.2f\n", balance);
                }
                else
                {
                    printf("Failed to check balance. Check account number or PIN.\n");
                }
            }
            wait_for_enter();
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

            if (!is_valid_account_no(account_no))
            {
                printf("Invalid account number format.\n");
                wait_for_enter();
                break;
            }

            snprintf(request, MAX_MSG_LEN, "GET_STATEMENT %s %s 0", account_no, pin);
            if (send_request(request, response, MAX_MSG_LEN))
            {
                char status[32];
                if (sscanf(response, "%s", status) == 1 && strcmp(status, "OK") == 0)
                {
                    printf("\nAccount Statement:\n%s\n", response + 3); // Skip "OK "
                }
                else
                {
                    printf("Failed to get statement. Check account number or PIN.\n");
                }
            }
            wait_for_enter();
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

            if (!is_valid_account_no(account_no))
            {
                printf("Invalid account number format.\n");
                wait_for_enter();
                break;
            }

            snprintf(request, MAX_MSG_LEN, "CLOSE_ACCOUNT %s %s 0", account_no, pin);
            if (send_request(request, response, MAX_MSG_LEN))
            {
                char status[32];
                double final_balance;
                if (sscanf(response, "%s %lf", status, &final_balance) == 2 &&
                    strcmp(status, "OK") == 0)
                {
                    printf("Account closed successfully.\n");
                    printf("Final balance returned: %.2f\n", final_balance);
                }
                else
                {
                    printf("Failed to close account. Check account number or PIN.\n");
                }
            }
            wait_for_enter();
            break;
        }

        default:
            printf("Invalid choice. Please try again.\n");
            wait_for_enter();
        }
    }
}

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main()
{
    printf("Banking Client - TCP Version\n");
    printf("===========================\n\n");

    initialize_client();
    handle_user_input();

    close(sockfd);
    return 0;
}