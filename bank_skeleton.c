/**
 * Banking Application - Version 1: Single Program
 * 
 * This is the monolithic version that combines:
 * - User interface (menu-driven)
 * - Business logic (register, deposit, withdraw, check balance)
 * - File-based persistence
 * 
 * The design is modular to facilitate future client-server splitting:
 * - UI functions can be moved to client.c
 * - Business logic and persistence can be moved to server.c
 * - Message parsing functions will be used for client-server communication
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #undef fileno
    #define fileno _fileno
#else
    #include <unistd.h>
    #include <sys/file.h>
#endif

/* Constants */
#define DB_FILENAME "data.txt"
#define MAX_LINE_LEN 256
#define MAX_MSG_LEN 256
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

/* Message operation codes - will be used for client-server communication */
#define OP_REGISTER "REGISTER"
#define OP_DEPOSIT "DEPOSIT"
#define OP_WITHDRAW "WITHDRAW"
#define OP_CHECK "CHECK_BALANCE"

/* Response codes - will be used for client-server communication */
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_ACCT_EXISTS "ACCOUNT_EXISTS" 
#define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
#define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
#define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
#define RESP_INVALID_REQUEST "INVALID_REQUEST"

/* Function prototypes */

/* UI Functions - will move to client.c in future versions */
void display_menu();
void handle_user_input();
void clear_input_buffer();

/* Message Functions - will be used for client-server communication */
char* create_message(const char* operation, const char* account_no, double amount);
bool parse_message(const char* message, char* operation, char* account_no, double* amount);
char* create_response(const char* status, double balance);
bool parse_response(const char* response, char* status, double* balance);

/* Business Logic Functions - will move to server.c in future versions */
char* process_request(const char* request);
char* register_account(const char* account_no);
char* deposit(const char* account_no, double amount);
char* withdraw(const char* account_no, double amount);
char* check_balance(const char* account_no);

/* File I/O Functions - will move to server.c in future versions */
bool account_exists(const char* account_no);
bool get_balance(const char* account_no, double* balance);
bool update_balance(const char* account_no, double new_balance);
bool add_account(const char* account_no, double initial_balance);

/* Utility Functions */
void trim(char* str);
bool is_valid_account_no(const char* account_no);
bool is_valid_amount(double amount);

/* Main Function */
int main() {
    printf("Banking Application - Version 1\n");
    printf("===============================\n\n");
    
    handle_user_input();
    
    return 0;
}

/* UI Functions Implementation */

void display_menu() {
    printf("\nBANKING MENU\n");
    printf("============\n");
    printf("1. Register New Account\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Check Balance\n");
    printf("0. Exit\n");
    printf("\nEnter choice: ");
}

void handle_user_input() {
    int choice;
    char account_no[MAX_ACCT_LEN];
    char amount_str[MAX_AMT_LEN];
    double amount;
    char request[MAX_MSG_LEN];
    char* response;
    
    while (1) {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();
        
        if (choice == 0) {
            printf("Thank you for using our banking service. Goodbye!\n");
            break;
        }
        
        switch (choice) {
            case 1: // Register
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);
                
                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }
                
                strcpy(request, create_message(OP_REGISTER, account_no, 0));
                response = process_request(request);
                printf("%s\n", response);
                free(response);
                break;
                
            case 2: // Deposit
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);
                
                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }
                
                printf("Enter Amount: ");
                if (fgets(amount_str, MAX_AMT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                amount = atof(amount_str);
                
                if (!is_valid_amount(amount)) {
                    printf("Invalid amount.\n");
                    continue;
                }
                
                strcpy(request, create_message(OP_DEPOSIT, account_no, amount));
                response = process_request(request);
                printf("%s\n", response);
                free(response);
                break;
                
            case 3: // Withdraw
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);
                
                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }
                
                printf("Enter Amount: ");
                if (fgets(amount_str, MAX_AMT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                amount = atof(amount_str);
                
                if (!is_valid_amount(amount)) {
                    printf("Invalid amount.\n");
                    continue;
                }
                
                strcpy(request, create_message(OP_WITHDRAW, account_no, amount));
                response = process_request(request);
                printf("%s\n", response);
                free(response);
                break;
                
            case 4: // Check Balance
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);
                
                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }
                
                strcpy(request, create_message(OP_CHECK, account_no, 0));
                response = process_request(request);
                printf("%s\n", response);
                free(response);
                break;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* Message Functions Implementation */

char* create_message(const char* operation, const char* account_no, double amount) {
    static char message[MAX_MSG_LEN];
    
    // Format: OPERATION account_no amount
    if (strcmp(operation, OP_REGISTER) == 0) {
        snprintf(message, MAX_MSG_LEN, "%s %s", operation, account_no);
    } else if (strcmp(operation, OP_CHECK) == 0) {
        snprintf(message, MAX_MSG_LEN, "%s %s", operation, account_no);
    } else {
        snprintf(message, MAX_MSG_LEN, "%s %s %.2f", operation, account_no, amount);
    }
    
    return message;
}

bool parse_message(const char* message, char* operation, char* account_no, double* amount) {
    int result;
    
    // Assume message format: OPERATION account_no [amount]
    // For REGISTER and CHECK_BALANCE, amount is optional
    
    // Try parsing with amount
    result = sscanf(message, "%s %s %lf", operation, account_no, amount);
    if (result >= 2) { // At least operation and account_no were parsed
        return true;
    }
    
    return false;
}

char* create_response(const char* status, double balance) {
    char* response = malloc(MAX_MSG_LEN);
    if (!response) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    // Format: STATUS balance
    snprintf(response, MAX_MSG_LEN, "%s %.2f", status, balance);
    
    return response;
}

bool parse_response(const char* response, char* status, double* balance) {
    int result = sscanf(response, "%s %lf", status, balance);
    return (result >= 1); // At least status was parsed
}

/* Business Logic Functions Implementation */

char* process_request(const char* request) {
    char operation[MAX_LINE_LEN];
    char account_no[MAX_ACCT_LEN];
    double amount = 0;
    
    if (!parse_message(request, operation, account_no, &amount)) {
        return create_response(RESP_INVALID_REQUEST, 0);
    }
    
    if (strcmp(operation, OP_REGISTER) == 0) {
        return register_account(account_no);
    } else if (strcmp(operation, OP_DEPOSIT) == 0) {
        return deposit(account_no, amount);
    } else if (strcmp(operation, OP_WITHDRAW) == 0) {
        return withdraw(account_no, amount);
    } else if (strcmp(operation, OP_CHECK) == 0) {
        return check_balance(account_no);
    } else {
        return create_response(RESP_INVALID_REQUEST, 0);
    }
}

char* register_account(const char* account_no) {
    if (account_exists(account_no)) {
        return create_response(RESP_ACCT_EXISTS, 0);
    }
    
    if (add_account(account_no, 0)) {
        return create_response(RESP_OK, 0);
    } else {
        return create_response(RESP_ERROR, 0);
    }
}

char* deposit(const char* account_no, double amount) {
    double balance = 0;
    
    if (!is_valid_amount(amount)) {
        return create_response(RESP_INVALID_AMOUNT, 0);
    }
    
    if (!account_exists(account_no)) {
        return create_response(RESP_ACCT_NOT_FOUND, 0);
    }
    
    if (!get_balance(account_no, &balance)) {
        return create_response(RESP_ERROR, 0);
    }
    
    balance += amount;
    
    if (update_balance(account_no, balance)) {
        return create_response(RESP_OK, balance);
    } else {
        return create_response(RESP_ERROR, 0);
    }
}

char* withdraw(const char* account_no, double amount) {
    double balance = 0;
    
    if (!is_valid_amount(amount)) {
        return create_response(RESP_INVALID_AMOUNT, 0);
    }
    
    if (!account_exists(account_no)) {
        return create_response(RESP_ACCT_NOT_FOUND, 0);
    }
    
    if (!get_balance(account_no, &balance)) {
        return create_response(RESP_ERROR, 0);
    }
    
    if (balance < amount) {
        return create_response(RESP_INSUFFICIENT_FUNDS, balance);
    }
    
    balance -= amount;
    
    if (update_balance(account_no, balance)) {
        return create_response(RESP_OK, balance);
    } else {
        return create_response(RESP_ERROR, 0);
    }
}

char* check_balance(const char* account_no) {
    double balance = 0;
    
    if (!account_exists(account_no)) {
        return create_response(RESP_ACCT_NOT_FOUND, 0);
    }
    
    if (get_balance(account_no, &balance)) {
        return create_response(RESP_OK, balance);
    } else {
        return create_response(RESP_ERROR, 0);
    }
}

/* File I/O Functions Implementation */

// Cross-platform file locking function
static void lock_file(FILE* file, bool exclusive) {
#ifdef _WIN32
    // Windows file locking
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    OVERLAPPED overlapped = {0};
    
    if (exclusive) {
        LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlapped);
    } else {
        LockFileEx(hFile, 0, 0, MAXDWORD, MAXDWORD, &overlapped);
    }
#else
    // UNIX/Linux file locking
    if (exclusive) {
        flock(fileno(file), LOCK_EX);
    } else {
        flock(fileno(file), LOCK_SH);
    }
#endif
}

// Cross-platform file unlocking function
static void unlock_file(FILE* file) {
#ifdef _WIN32
    // Windows file unlocking
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    OVERLAPPED overlapped = {0};
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
#else
    // UNIX/Linux file unlocking
    flock(fileno(file), LOCK_UN);
#endif
}

bool account_exists(const char* account_no) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        // If file doesn't exist, no accounts exist
        return false;
    }
    
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN];
    bool found = false;
    
    // Lock file for reading
    lock_file(file, false);  // Shared lock for reading
    
    while (fgets(line, MAX_LINE_LEN, file)) {
        if (sscanf(line, "%s", file_acct) == 1) {
            if (strcmp(file_acct, account_no) == 0) {
                found = true;
                break;
            }
        }
    }
    
    // Unlock file
    unlock_file(file);
    fclose(file);
    
    return found;
}

bool get_balance(const char* account_no, double* balance) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        return false;
    }
    
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN];
    double file_balance;
    bool found = false;
    
    // Lock file for reading
    lock_file(file, false);  // Shared lock for reading
    
    while (fgets(line, MAX_LINE_LEN, file)) {
        if (sscanf(line, "%s %lf", file_acct, &file_balance) == 2) {
            if (strcmp(file_acct, account_no) == 0) {
                *balance = file_balance;
                found = true;
                break;
            }
        }
    }
    
    // Unlock file
    unlock_file(file);
    fclose(file);
    
    return found;
}

bool update_balance(const char* account_no, double new_balance) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        return false;
    }
    
    FILE* temp = fopen("temp.txt", "w");
    if (!temp) {
        fclose(file);
        return false;
    }
    
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN];
    double file_balance;
    bool updated = false;
    
    // Lock both files for writing
    lock_file(file, true);  // Exclusive lock for writing
    lock_file(temp, true);  // Exclusive lock for writing
    
    while (fgets(line, MAX_LINE_LEN, file)) {
        if (sscanf(line, "%s %lf", file_acct, &file_balance) == 2) {
            if (strcmp(file_acct, account_no) == 0) {
                fprintf(temp, "%s %.2f\n", account_no, new_balance);
                updated = true;
            } else {
                fputs(line, temp);
            }
        } else {
            fputs(line, temp);
        }
    }
    
    // Unlock files
    unlock_file(file);
    unlock_file(temp);
    
    fclose(file);
    fclose(temp);
    
    // Replace old file with new one
    if (remove(DB_FILENAME) != 0) {
        perror("Error removing file");
        return false;
    }
    
    if (rename("temp.txt", DB_FILENAME) != 0) {
        perror("Error renaming file");
        return false;
    }
    
    return updated;
}

bool add_account(const char* account_no, double initial_balance) {
    FILE* file = fopen(DB_FILENAME, "a+");
    if (!file) {
        // Create file if it doesn't exist
        file = fopen(DB_FILENAME, "w");
        if (!file) {
            perror("Error creating database file");
            return false;
        }
    }
    
    // Lock file for writing
    lock_file(file, true);  // Exclusive lock for writing
    
    // Add account to end of file
    fprintf(file, "%s %.2f\n", account_no, initial_balance);
    
    // Ensure data is written to disk
    fflush(file);
    
    // Unlock file
    unlock_file(file);
    fclose(file);
    
    return true;
}

/* Utility Functions Implementation */

void trim(char* str) {
    if (!str) return;
    
    // Trim leading space
    char* start = str;
    while (isspace((unsigned char)*start)) start++;
    
    if (*start == 0) { // All spaces?
        str[0] = '\0';
        return;
    }
    
    // Trim trailing space
    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    // Move the string to the front if needed
    if (start > str) {
        memmove(str, start, strlen(start) + 1);
    }
}

bool is_valid_account_no(const char* account_no) {
    // Basic validation - ensure account number is not empty and only contains digits
    if (!account_no || strlen(account_no) == 0) {
        return false;
    }
    
    for (size_t i = 0; i < strlen(account_no); i++) {
        if (!isdigit(account_no[i])) {
            return false;
        }
    }
    
    return true;
}

bool is_valid_amount(double amount) {
    // Basic validation - ensure amount is positive
    return amount > 0;
}