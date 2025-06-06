/* client.c - Menu-based client UI */
#include "common.h"

void display_menu() {
    printf("\n=== Banking Menu ===\n");
    printf("1. Register\n2. Deposit\n3. Withdraw\n4. Check Balance\n5. Statement\n6. Close Account\n7. Exit\n> ");
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char account_no[MAX_ACCT_LEN], pin[8], buffer[MAX_MSG_LEN], response[MAX_MSG_LEN], status[32];
    double amount, balance;

    while (1) {
        display_menu();
        int choice;
        scanf("%d", &choice); getchar();
        if (choice == 7) break;

        if (choice == 1) {
            printf("Enter account number: ");
            fgets(account_no, sizeof(account_no), stdin);
            strtok(account_no, "\n");
            strcpy(buffer, create_message(OP_REGISTER, account_no, 0));
        } else if (choice == 2 || choice == 3) {
            printf("Enter account number: ");
            fgets(account_no, sizeof(account_no), stdin); strtok(account_no, "\n");
            printf("Enter PIN: ");
            fgets(pin, sizeof(pin), stdin); strtok(pin, "\n");
            printf("Enter amount: ");
            scanf("%lf", &amount); getchar();
            snprintf(buffer, sizeof(buffer), "%s %s %s %.2f",
                (choice == 2) ? OP_DEPOSIT : OP_WITHDRAW, account_no, pin, amount);
        } else if (choice == 4) {
            printf("Enter account number: ");
            fgets(account_no, sizeof(account_no), stdin); strtok(account_no, "\n");
            printf("Enter PIN: ");
            fgets(pin, sizeof(pin), stdin); strtok(pin, "\n");
            snprintf(buffer, sizeof(buffer), "%s %s %s", OP_CHECK, account_no, pin);
        } else if (choice == 5) {
            printf("Enter account number: ");
            fgets(account_no, sizeof(account_no), stdin); strtok(account_no, "\n");
            printf("Enter PIN: ");
            fgets(pin, sizeof(pin), stdin); strtok(pin, "\n");
            snprintf(buffer, sizeof(buffer), "%s %s %s", OP_STATEMENT, account_no, pin);
        } else if (choice == 6) {
            printf("Enter account number: ");
            fgets(account_no, sizeof(account_no), stdin); strtok(account_no, "\n");
            printf("Enter PIN: ");
            fgets(pin, sizeof(pin), stdin); strtok(pin, "\n");
            snprintf(buffer, sizeof(buffer), "%s %s %s", OP_CLOSE, account_no, pin);
        } else {
            printf("Invalid choice.\n");
            continue;
        }

        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        socklen_t addr_len = sizeof(serv_addr);
        int n = recvfrom(sockfd, response, MAX_MSG_LEN-1, 0,
                         (struct sockaddr*)&serv_addr, &addr_len);
        response[n] = '\0';

        // Print server response (parse as needed)
        printf("Server: %s\n", response);
    }
    close(sockfd);
    return 0;
}
