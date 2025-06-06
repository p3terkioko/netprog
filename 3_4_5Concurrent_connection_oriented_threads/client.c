#include "common.h"

void display_menu() {
    printf("\n=== Banking Menu ===\n");
    printf("1. Register Account\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Check Balance\n");
    printf("5. Exit\n");
    printf("Enter choice: ");
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[MAX_MSG_LEN];
    
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to banking server\n");
    
    int choice;
    char account_no[MAX_ACCT_LEN];
    char pin[8];
    double amount;
    
    while (1) {
        display_menu();
        scanf("%d", &choice);
        getchar(); // Consume newline
        
        if (choice == 5) break;
        
        switch (choice) {
            case 1: // Register
                printf("Enter account number: ");
                fgets(account_no, sizeof(account_no), stdin);
                trim_newline(account_no);
                
                snprintf(buffer, sizeof(buffer), "%s %s", OP_REGISTER, account_no);
                break;
                
            case 2: // Deposit
                printf("Enter account number: ");
                fgets(account_no, sizeof(account_no), stdin);
                trim_newline(account_no);
                
                printf("Enter amount: ");
                scanf("%lf", &amount);
                getchar();
                
                snprintf(buffer, sizeof(buffer), "%s %s %.2lf", OP_DEPOSIT, account_no, amount);
                break;
                
            case 3: // Withdraw
                printf("Enter account number: ");
                fgets(account_no, sizeof(account_no), stdin);
                trim_newline(account_no);
                
                printf("Enter amount: ");
                scanf("%lf", &amount);
                getchar();
                
                snprintf(buffer, sizeof(buffer), "%s %s %.2lf", OP_WITHDRAW, account_no, amount);
                break;
                
            case 4: // Check Balance
                printf("Enter account number: ");
                fgets(account_no, sizeof(account_no), stdin);
                trim_newline(account_no);
                
                snprintf(buffer, sizeof(buffer), "%s %s", OP_CHECK, account_no);
                break;
                
            default:
                printf("Invalid choice\n");
                continue;
        }
        
        // Send request
        write(sockfd, buffer, strlen(buffer));
        
        // Receive response
        int valread = read(sockfd, buffer, sizeof(buffer)-1);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("Server response: %s\n", buffer);
        } else {
            printf("No response from server\n");
        }
    }
    
    close(sockfd);
    return 0;
}