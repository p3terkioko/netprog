/* client.c - Menu-based client UI */
#include "common.h"

void display_menu() {
    printf("\n=== Banking Menu ===\n");
    printf("1. Register\n2. Deposit\n3. Withdraw\n4. Check Balance\n5. Statement\n6. Close Account\n7. Exit\n> ");
}

int main() {
    int choice;
    char account_no[MAX_ACCT_LEN], pin[8], buffer[MAX_MSG_LEN], response[MAX_MSG_LEN], status[32];
    double amount, balance;

    while (1) {
        display_menu();
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

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(PORT),
            .sin_addr.s_addr = inet_addr("127.0.0.1")
        };
        connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        write(sock, buffer, strlen(buffer));
        int n = read(sock, response, sizeof(response)-1);
        response[n] = '\0';

        // Print server response (parse as needed)
        printf("Server: %s\n", response);

        close(sock);
    }
    return 0;
}
