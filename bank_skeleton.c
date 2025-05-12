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
#include <time.h>

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

// New/extended function prototypes
bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit);
bool close_account(const char* account_no, const char* pin);
bool withdraw_extended(const char* account_no, const char* pin, double amount);
bool deposit_extended(const char* account_no, const char* pin, double amount);
bool balance(const char* account_no, double* out_balance);
bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN]);
void log_transaction(const char* account_no, const char* type, double amount, double balance);

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
    printf("5. Statement\n");
    printf("6. Close Account\n"); // <-- Add this line
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
            case 1: { // Register New Account
                char name[64], national_id[32], account_type[16];
                char acct_no[MAX_ACCT_LEN], pin[8];
                double initial_deposit = 0.0;
                char deposit_str[32];

                printf("Enter Name: ");
                if (fgets(name, sizeof(name), stdin) == NULL) break;
                trim(name);

                printf("Enter National ID: ");
                if (fgets(national_id, sizeof(national_id), stdin) == NULL) break;
                trim(national_id);

                printf("Enter Account Type (savings/checking): ");
                if (fgets(account_type, sizeof(account_type), stdin) == NULL) break;
                trim(account_type);

                printf("Enter Initial Deposit (minimum 1000): ");
                if (fgets(deposit_str, sizeof(deposit_str), stdin) == NULL) break;
                initial_deposit = atof(deposit_str);

                if (initial_deposit < 1000) {
                    printf("Initial deposit must be at least 1000.\n");
                    break;
                }

                if (!open_account(name, national_id, account_type, acct_no, pin, initial_deposit)) {
                    printf("Account registration failed.\n");
                } else {
                    printf("Account created! Number: %s, PIN: %s\n", acct_no, pin);
                }
                break;
            }
                
            case 2: { // Deposit
                char pin[8];
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);

                printf("Enter PIN: ");
                if (fgets(pin, sizeof(pin), stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(pin);

                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }

                printf("Enter Amount (minimum 500): ");
                if (fgets(amount_str, MAX_AMT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                amount = atof(amount_str);

                if (amount < 500) {
                    printf("Minimum deposit is 500.\n");
                    continue;
                }

                if (!deposit_extended(account_no, pin, amount)) {
                    printf("Deposit failed. Check account number or PIN.\n");
                } else {
                    printf("Deposit successful.\n");
                }
                break;
            }

            case 3: { // Withdraw
                char pin[8];
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);

                printf("Enter PIN: ");
                if (fgets(pin, sizeof(pin), stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(pin);

                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n");
                    continue;
                }

                printf("Enter Amount (minimum 500, must leave at least 1000 in account): ");
                if (fgets(amount_str, MAX_AMT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                amount = atof(amount_str);

                if (amount < 500) {
                    printf("Minimum withdrawal is 500.\n");
                    continue;
                }

                if (!withdraw_extended(account_no, pin, amount)) {
                    printf("Withdrawal failed. Check account number, PIN, or balance.\n");
                } else {
                    printf("Withdrawal successful.\n");
                }
                break;
            }
                
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

            case 5: { // Statement
                char pin[8];
                char transactions[5][MAX_LINE_LEN];

                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);

                printf("Enter PIN: ");
                if (fgets(pin, sizeof(pin), stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(pin);

                if (!statement(account_no, pin, transactions)) {
                    printf("Could not retrieve statement. Check account number or PIN.\n");
                } else {
                    printf("Last 5 transactions:\n");
                    for (int i = 0; i < 5; ++i) {
                        if (strlen(transactions[i]) > 0)
                            printf("%s\n", transactions[i]);
                    }
                }
                break;
            }

            case 6: { // Close Account
                char pin[8];
                printf("Enter Account Number: ");
                if (fgets(account_no, MAX_ACCT_LEN, stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(account_no);

                printf("Enter PIN: ");
                if (fgets(pin, sizeof(pin), stdin) == NULL) {
                    printf("Error reading input.\n");
                    continue;
                }
                trim(pin);

                if (close_account(account_no, pin)) {
                    printf("Account closed successfully.\n");
                } else {
                    printf("Failed to close account. Check account number or PIN.\n");
                }
                break;
            }
                
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
    char file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double file_balance;
    bool updated = false;

    // Lock both files for writing
    lock_file(file, true);  // Exclusive lock for writing
    lock_file(temp, true);  // Exclusive lock for writing

    while (fgets(line, MAX_LINE_LEN, file)) {
        // Try to parse full record
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &file_balance) == 6) {
            if (strcmp(file_acct, account_no) == 0) {
                // Write updated balance, keep all other fields
                fprintf(temp, "%s %s %s %s %s %.2f\n", file_acct, file_pin, name, national_id, account_type, new_balance);
                updated = true;
            } else {
                fputs(line, temp);
            }
        } else {
            // If not a full record, just copy as is
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

// New/extended function implementations

// Helper to generate a random 4-digit PIN
static void generate_pin(char* pin_out) {
    srand((unsigned int)time(NULL) ^ (unsigned int)rand());
    int pin = 1000 + rand() % 9000;
    sprintf(pin_out, "%04d", pin);
}

// Helper to generate a unique account number (incremental)
static void generate_account_no(char* acct_out) {
    FILE* file = fopen(DB_FILENAME, "r");
    int max_acct = 100000;
    char line[MAX_LINE_LEN], acct[MAX_ACCT_LEN];
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "%s", acct) == 1) {
                int n = atoi(acct);
                if (n > max_acct) max_acct = n;
            }
        }
        fclose(file);
    }
    sprintf(acct_out, "%d", max_acct + 1);
}

// Creates an account with minimum 1k, stores name, national ID, type, generates account number and PIN
bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit) {
    if (!name || !national_id || !account_type || !out_account_no || !out_pin) return false;
    if (initial_deposit < 1000) return false;

    // Validate account type
    if (strcmp(account_type, "savings") != 0 && strcmp(account_type, "checking") != 0) {
        printf("Invalid account type. Must be 'savings' or 'checking'.\n");
        return false;
    }

    // Generate unique account number and PIN
    generate_account_no(out_account_no);
    generate_pin(out_pin);

    // Open file for appending
    FILE* file = fopen(DB_FILENAME, "a+");
    if (!file) {
        perror("Error opening database file");
        return false;
    }

    // Lock file for writing
    lock_file(file, true);

    // Write account details: account_no pin name national_id account_type balance
    fprintf(file, "%s %s %s %s %s %.2f\n", out_account_no, out_pin, name, national_id, account_type, initial_deposit);

    fflush(file);
    unlock_file(file);
    fclose(file);

    return true;
}

// Closes an account (marks as closed or removes from DB)
bool close_account(const char* account_no, const char* pin) {
    // Remove the account from the database if account_no and pin match
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) return false;
    FILE* temp = fopen("temp.txt", "w");
    if (!temp) {
        fclose(file);
        return false;
    }

    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool closed = false;

    // Lock files for writing
    lock_file(file, true);
    lock_file(temp, true);

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0) {
                closed = true; // Skip writing this account (i.e., delete)
                continue;
            }
        }
        fputs(line, temp);
    }

    unlock_file(file);
    unlock_file(temp);
    fclose(file);
    fclose(temp);

    // Replace old file with new one
    remove(DB_FILENAME);
    rename("temp.txt", DB_FILENAME);

    // Delete the transaction file for this account
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    remove(filename);

    return closed;
}

// Withdraws, leaving at least 1k, in units of >= 500
bool withdraw_extended(const char* account_no, const char* pin, double amount) {
    if (amount < 500) return false;

    FILE* db = fopen(DB_FILENAME, "r+");
    if (!db) return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double balance = 0;
    bool found = false;

    // Find account and verify PIN
    while (fgets(line, sizeof(line), db)) {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &balance) == 6) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0) {
                found = true;
                break;
            }
        }
    }
    fclose(db);
    if (!found) return false;

    if (balance - amount < 1000) return false;

    // Update balance
    if (!update_balance(account_no, balance - amount)) return false;

    // Log transaction
    log_transaction(account_no, "WITHDRAW", amount, balance - amount);

    return true;
}

// Deposit at least 500
bool deposit_extended(const char* account_no, const char* pin, double amount) {
    if (amount < 500) return false;

    FILE* db = fopen(DB_FILENAME, "r");
    if (!db) return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double balance = 0;
    bool found = false;

    // Find account and verify PIN
    while (fgets(line, sizeof(line), db)) {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &balance) == 6) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0) {
                found = true;
                break;
            }
        }
    }
    fclose(db);
    if (!found) return false;

    // Update balance
    if (!update_balance(account_no, balance + amount)) return false;

    // Log transaction
    log_transaction(account_no, "DEPOSIT", amount, balance + amount);

    return true;
}

// Returns the balance in the account
bool balance(const char* account_no, double* out_balance) {
    // Fetch balance for the given account number
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN];
    double file_balance;
    bool found = false;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %*s %*s %*s %*s %lf", file_acct, &file_balance) == 2) {
            if (strcmp(file_acct, account_no) == 0) {
                *out_balance = file_balance;
                found = true;
                break;
            }
        }
    }
    fclose(file);
    return found;
}

// Returns last five transactions (debit/credit) for the account
bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN]) {
    // Verify PIN
    FILE* db = fopen(DB_FILENAME, "r");
    if (!db) return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool found = false;
    while (fgets(line, sizeof(line), db)) {

        // Match the format: account_no pin name national_id account_type balance
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0) {
                found = true;
                break;
            }
        }
    }
    fclose(db);
    if (!found) return false;

    // Read last 5 transactions from the account's file
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE* tf = fopen(filename, "r");
    if (!tf) return false;

    char all[1000][MAX_LINE_LEN];
    int count = 0;
    while (fgets(line, sizeof(line), tf)) {
        strncpy(all[count++], line, MAX_LINE_LEN);
    }
    fclose(tf);

    int start = count > 5 ? count - 5 : 0;
    int idx = 0;
    for (int i = start; i < count; ++i, ++idx) {
        strncpy(transactions[idx], all[i], MAX_LINE_LEN);
    }
    for (; idx < 5; ++idx) transactions[idx][0] = '\0';

    return true;
}

void log_transaction(const char* account_no, const char* type, double amount, double balance) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE* file = fopen(filename, "a");
    if (!file) return;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);
    fprintf(file, "%s %.2f %.2f %s\n", type, amount, balance, timebuf);
    fclose(file);
}