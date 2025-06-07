// client_ui.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bank_logic.h"

#define MAX_LINE_LEN 256
#define MAX_BUFFER 1024

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

void send_request_udp(int sockfd, struct sockaddr_in *server_addr, const char* request, char* response) {
    socklen_t addr_len = sizeof(*server_addr);

    sendto(sockfd, request, strlen(request), 0,
           (struct sockaddr *)server_addr, addr_len);

    ssize_t n = recvfrom(sockfd, response, MAX_BUFFER - 1, 0,
                         NULL, NULL);
    if (n < 0) {
        perror("recvfrom failed");
        strcpy(response, "ERROR|Timeout or no response");
    } else {
        response[n] = '\0';
    }
}

void handle_user_input(int sockfd, struct sockaddr_in *server_addr) {
    int choice;
    char account_no[MAX_ACCT_LEN], amount_str[MAX_AMT_LEN], pin[8];
    char request[MAX_MSG_LEN], response[MAX_BUFFER];

    while (1) {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        if (choice == 0) {
            printf("Thank you for using our banking service. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1: {
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
                    send_request_udp(sockfd, server_addr, request, response);
                    printf("%s\n", response);
                }
                wait_for_enter();
                break;
            }

            case 2: {
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
                    send_request_udp(sockfd, server_addr, request, response);
                    printf("%s\n", response);
                }
                wait_for_enter();
                break;
            }

            case 3: {
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
                send_request_udp(sockfd, server_addr, request, response);
                printf("%s\n", response);
                wait_for_enter();
                break;
            }

            case 4: {
                clear_screen();
                printf("=== CHECK BALANCE ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "BALANCE|%s|%s", account_no, pin);
                send_request_udp(sockfd, server_addr, request, response);
                printf("Balance: %s\n", response);
                wait_for_enter();
                break;
            }

            case 5: {
                clear_screen();
                printf("=== STATEMENT ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "STATEMENT|%s|%s", account_no, pin);
                send_request_udp(sockfd, server_addr, request, response);
                printf("Statement:\n%s\n", response);
                wait_for_enter();
                break;
            }

            case 6: {
                clear_screen();
                printf("=== CLOSE ACCOUNT ===\n");

                printf("Enter Account Number: ");
                fgets(account_no, sizeof(account_no), stdin); trim(account_no);

                printf("Enter PIN: ");
                fgets(pin, sizeof(pin), stdin); trim(pin);

                snprintf(request, sizeof(request), "CLOSE|%s|%s", account_no, pin);
                send_request_udp(sockfd, server_addr, request, response);
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
