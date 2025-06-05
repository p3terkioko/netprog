// client_ui.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "bank_logic.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#define MAX_LINE_LEN 256

void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void display_menu() {
    clear_screen();
    printf("\nBANKING MENU\n");
    printf("============\n");
    printf("1. Register New Account\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Check Balance\n");
    printf("5. Statement\n");
    printf("6. Close Account\n");
    printf("0. Exit\n");
    printf("\nEnter choice: ");
}

void wait_for_enter() {
    printf("\nPress ENTER to return to the main menu...");
    while (getchar() != '\n');
}

void send_request(int sockfd, const char* request, char* response) {
    send(sockfd, request, strlen(request), 0);
    recv(sockfd, response, MAX_MSG_LEN, 0);
    response[MAX_MSG_LEN - 1] = '\0'; // Null-terminate
}

void handle_user_input(int sockfd) {
    int choice;
    char account_no[MAX_ACCT_LEN], amount_str[MAX_AMT_LEN], pin[8];
    char request[MAX_MSG_LEN], response[MAX_MSG_LEN];

    while (1) {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');  // clear input buffer

        if (choice == 0) {
            printf("Thank you for using our banking service. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1: { // Register
                char name[64], national_id[32], account_type[16], deposit_str[32];
                double initial_deposit;
                clear_screen();
                printf("=== ACCOUNT REGISTRATION ===\n");

                printf("Enter Name: ");
                fgets(name, sizeof(name), stdin); trim(name);

                printf("Enter National ID: ");
                fgets(national_id, sizeof(national_id), stdin); trim(national_id);

                printf("Enter Account Type (savings/checking): ");
                fgets(account_type, sizeof(account_type), stdin); trim(account_type);

                printf("Enter Initial Deposit (minimum 1000): ");
                fgets(deposit_str, sizeof(deposit_str), stdin); trim(deposit_str);
                initial_deposit = atof(deposit_str);

                if (initial_deposit < 1000) {
                    printf("Initial deposit must be at least 1000.\n");
                } else {
                    snprintf(request, sizeof(request), "OPEN|%s|%s|%s|%.2f", name, national_id, account_type, initial_deposit);
                    send_request(sockfd, request, response);
                    printf("%s\n", response);
                }
                wait_for_enter();
                break;
            }

            case 2: { // Deposit
                clear_screen();
                printf("=== DEPOSIT ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                printf("Enter Amount (min 500): ");
                fgets(amount_str, sizeof(amount_str), stdin); trim(amount_str);
                double amt = atof(amount_str);

                if (amt < 500) {
                    printf("Minimum deposit is 500.\n");
                } else {
                    snprintf(request, sizeof(request), "DEPOSIT|%s|%s|%.2f", account_no, pin, amt);
                    send_request(sockfd, request, response);
                    printf("%s\n", response);
                }
                wait_for_enter();
                break;
            }

            case 3: { // Withdraw
                clear_screen();
                printf("=== WITHDRAW ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                printf("Enter Amount: ");
                fgets(amount_str, sizeof(amount_str), stdin); trim(amount_str);
                double amt = atof(amount_str);

                snprintf(request, sizeof(request), "WITHDRAW|%s|%s|%.2f", account_no, pin, amt);
                send_request(sockfd, request, response);
                printf("%s\n", response);
                wait_for_enter();
                break;
            }

            case 4: { // Check Balance
                clear_screen();
                printf("=== CHECK BALANCE ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "CHECK|%s|%s", account_no, pin);
                send_request(sockfd, request, response);
                printf("Balance: %s\n", response);
                wait_for_enter();
                break;
            }

            case 5: { // Statement
                clear_screen();
                printf("=== STATEMENT ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "STATEMENT|%s|%s", account_no, pin);
                send_request(sockfd, request, response);
                printf("Statement:\n%s\n", response);
                wait_for_enter();
                break;
            }

            case 6: { // Close Account
                clear_screen();
                printf("=== CLOSE ACCOUNT ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "CLOSE|%s|%s", account_no, pin);
                send_request(sockfd, request, response);
                printf("%s\n", response);
                wait_for_enter();
                break;
            }

            default:
                printf("Invalid choice.\n");
                wait_for_enter();
                break;
        }
    }
}
