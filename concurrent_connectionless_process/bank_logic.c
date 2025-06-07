/*
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
 #include "bank_logic.h"
 
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
 
 // Constants
 #define DB_FILENAME "data.txt"
 #define MAX_LINE_LEN 256
 #define MAX_MSG_LEN 256
 #define MAX_ACCT_LEN 16
 #define MAX_AMT_LEN 16
 
 // Message operation codes - will be used for client-server communication
 #define OP_REGISTER "REGISTER"
 #define OP_DEPOSIT "DEPOSIT"
 #define OP_WITHDRAW "WITHDRAW"
 #define OP_CHECK "CHECK_BALANCE"
 
 // Response codes - will be used for client-server communication
 #define RESP_OK "OK"
 #define RESP_ERROR "ERROR"
 #define RESP_ACCT_EXISTS "ACCOUNT_EXISTS"
 #define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
 #define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
 #define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
 #define RESP_INVALID_REQUEST "INVALID_REQUEST"
 
 // Function prototypes
 
 // UI Functions - will move to client.c in future versions
 void display_menu();
 void handle_user_input();
 void clear_input_buffer();
 void clear_screen();
 void wait_for_enter();
 
 // Message Functions - will be used for client-server communication
 char* create_message(const char* operation, const char* account_no, double amount);
 bool parse_message(const char* message, char* operation, char* account_no, double* amount);
 char* create_response(const char* status, double balance);
 bool parse_response(const char* response, char* status, double* balance);
 
 // Business Logic Functions - will move to server.c in future versions
 char* process_request(const char* request);
 char* register_account(const char* account_no);
 char* deposit(const char* account_no, double amount);
 char* withdraw(const char* account_no, double amount);
 char* check_balance(const char* account_no);
 
 // File I/O Functions - will move to server.c in future versions
 bool account_exists(const char* account_no);
 bool get_balance(const char* account_no, double* balance);
 bool update_balance(const char* account_no, double new_balance);
 bool add_account(const char* account_no, double initial_balance);
 
 // Utility Functions
 void trim(char* str);
 bool is_valid_account_no(const char* account_no);
 bool is_valid_amount(double amount);
 
 // Extended function prototypes
 bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit);
 bool close_account(const char* account_no, const char* pin);
 bool withdraw_extended(const char* account_no, const char* pin, double amount);
 bool deposit_extended(const char* account_no, const char* pin, double amount);
 bool balance(const char* account_no, double* out_balance);
 bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN]);
 void log_transaction(const char* account_no, const char* type, double amount, double balance);
  
 // Message Functions Implementation
 
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
 
 // Business Logic Functions Implementation
 
char* process_request_extended(const char* request) {
    static char response[2048];
    char msg_copy[MAX_MSG_LEN];
    strncpy(msg_copy, request, MAX_MSG_LEN - 1);
    msg_copy[MAX_MSG_LEN - 1] = '\0';

    char *operation = strtok(msg_copy, "|");
    char *account_no = strtok(NULL, "|");
    char *pin = strtok(NULL, "|");
    char *amount_str = strtok(NULL, "|");
    double amount = 0;
    if (amount_str) amount = atof(amount_str);

    if (!operation || !account_no) {
        return create_response(RESP_INVALID_REQUEST, 0);
    }

    if (strcmp(operation, "BALANCE") == 0) {
        double bal;
        if (balance(account_no, &bal)) {
            snprintf(response, sizeof(response), "OK|%.2f", bal);
        } else {
            snprintf(response, sizeof(response), "ERROR|Could not retrieve balance");
        }
        return response;
    } else if (strcmp(operation, "STATEMENT") == 0) {
        char txns[5][MAX_LINE_LEN];
        if (statement(account_no, pin, txns)) {
            snprintf(response, sizeof(response), "OK|%s;%s;%s;%s;%s",
                     txns[0], txns[1], txns[2], txns[3], txns[4]);
        } else {
            snprintf(response, sizeof(response), "ERROR|Could not retrieve statement");
        }
        return response;
    } else if (strcmp(operation, "CLOSE") == 0) {
        if (close_account(account_no, pin)) {
            snprintf(response, sizeof(response), "OK|Account closed");
        } else {
            snprintf(response, sizeof(response), "ERROR|Closure failed");
        }
        return response;
    } else if (strcmp(operation, "WITHDRAW") == 0) {
        if (withdraw_extended(account_no, pin, amount)) {
            double bal;
            balance(account_no, &bal);
            snprintf(response, sizeof(response), "OK|%.2f", bal);
        } else {
            snprintf(response, sizeof(response), "ERROR|Transaction failed");
        }
        return response;
    } else if (strcmp(operation, "DEPOSIT") == 0) {
        if (deposit_extended(account_no, pin, amount)) {
            double bal;
            balance(account_no, &bal);
            snprintf(response, sizeof(response), "OK|%.2f", bal);
        } else {
            snprintf(response, sizeof(response), "ERROR|Transaction failed");
        }
        return response;
    } else if (strcmp(operation, "OPEN") == 0) {
        // Format: OPEN|Name|NID|Type|Amount
        char *name = account_no;
        char *nid = pin;
        char *type = amount_str;
        char *deposit_str = strtok(NULL, "|");

        if (!name || !nid || !type || !deposit_str) {
            snprintf(response, sizeof(response), "ERROR|Missing fields");
            return response;
        }

        double initial_deposit = atof(deposit_str);
        char acct_no[16], acct_pin[8];

        if (open_account(name, nid, type, acct_no, acct_pin, initial_deposit)) {
            snprintf(response, sizeof(response), "OK|Account: %s PIN: %s", acct_no, acct_pin);
        } else {
            snprintf(response, sizeof(response), "ERROR|Could not open account");
        }
        return response;
    }

    snprintf(response, sizeof(response), "ERROR|Unsupported operation");
    return response;
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
 
 // File I/O Functions Implementation
 
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
     // Unix/Linux file locking
     if (exclusive) {
         flock(fileno(file), LOCK_EX);
     } else {
         flock(fileno(file), LOCK_SH);
     }
 #endif
 }
 
 static void unlock_file(FILE* file) {
 #ifdef _WIN32
     HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
     OVERLAPPED overlapped = {0};
     UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
 #else
     flock(fileno(file), LOCK_UN);
 #endif
 }
 
 bool account_exists(const char* account_no) {
     FILE* file = fopen(DB_FILENAME, "r");
     if (!file) {
         // File doesn't exist, no accounts exist
         return false;
     }
     char line[MAX_LINE_LEN];
     char file_acct[MAX_ACCT_LEN];
     bool found = false;
     lock_file(file, false); // lock for reading
     while (fgets(line, MAX_LINE_LEN, file)) {
         if (sscanf(line, "%s", file_acct) == 1) {
             if (strcmp(file_acct, account_no) == 0) {
                 found = true;
                 break;
             }
         }
     }
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
     lock_file(file, false); // lock for reading
     while (fgets(line, MAX_LINE_LEN, file)) {
         // Skip to the 6th field (balance)
         if (sscanf(line, "%s %*s %*s %*s %*s %lf", file_acct, &file_balance) == 2) {
             if (strcmp(file_acct, account_no) == 0) {
                 *balance = file_balance;
                 found = true;
                 break;
             }
         }
     }
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
     lock_file(file, true);
     lock_file(temp, true);
     while (fgets(line, MAX_LINE_LEN, file)) {
         if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &file_balance) == 6) {
             if (strcmp(file_acct, account_no) == 0) {
                 fprintf(temp, "%s %s %s %s %s %.2f\n", file_acct, file_pin, name, national_id, account_type, new_balance);
                 updated = true;
             } else {
                 fputs(line, temp);
             }
         } else {
             fputs(line, temp);
         }
     }
     unlock_file(file);
     unlock_file(temp);
     fclose(file);
     fclose(temp);
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
         file = fopen(DB_FILENAME, "w");
         if (!file) {
             perror("Error creating database file");
             return false;
         }
     }
     lock_file(file, true);
     fprintf(file, "%s %.2f\n", account_no, initial_balance);
     fflush(file);
     unlock_file(file);
     fclose(file);
     return true;
 }
 
 // Utility Functions Implementation
 
 void trim(char* str) {
     if (!str) return;
     char* start = str; while (isspace((unsigned char)*start)) start++;
     if (!*start) { *str = 0; return; }
     char* end = start + strlen(start) - 1;
     while (end > start && isspace((unsigned char)*end)) end--;
     *(end + 1) = 0;
     if (start > str) memmove(str, start, strlen(start) + 1);
 }
 
 bool is_valid_account_no(const char* account_no) {
     if (!account_no || !*account_no) return false;
     for (const char* p = account_no; *p; ++p) if (!isdigit(*p)) return false;
     return true;
 }
 
 bool is_valid_amount(double amount) { return amount > 0; }
 
 // Extended function implementations
 
 // Helper to generate a random 4-digit PIN
 static void generate_pin(char* pin_out) {
     srand((unsigned int)time(NULL) ^ (unsigned int)rand());
     sprintf(pin_out, "%04d", 1000 + rand() % 9000);
 }
 
 // Helper to generate a unique account number (incremental)
 static void generate_account_no(char* acct_out) {
     FILE* file = fopen(DB_FILENAME, "r");
     int max_acct = 100000; char line[MAX_LINE_LEN], acct[MAX_ACCT_LEN];
     if (file) {
         while (fgets(line, sizeof(line), file))
             if (sscanf(line, "%s", acct) == 1) {
                 int n = atoi(acct); if (n > max_acct) max_acct = n;
             }
         fclose(file);
     }
     sprintf(acct_out, "%d", max_acct + 1);
 }
 
 // Creates an account with minimum 1k, stores name, national ID, type, generates account number and PIN
 bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit) {
     if (!name || !national_id || !account_type || !out_account_no || !out_pin) return false;
     if (initial_deposit < 1000) return false;
     if (strcmp(account_type, "savings") != 0 && strcmp(account_type, "checking") != 0) {
         printf("Invalid account type. Must be 'savings' or 'checking'.\n");
         return false;
     }
     generate_account_no(out_account_no);
     generate_pin(out_pin);
     FILE* file = fopen(DB_FILENAME, "a+");
     if (!file) {
         perror("Error opening database file");
         return false;
     }
     lock_file(file, true);
     fprintf(file, "%s %s %s %s %s %.2f\n", out_account_no, out_pin, name, national_id, account_type, initial_deposit);
     fflush(file);
     unlock_file(file);
     fclose(file);
     return true;
 }
 
 // Closes an account (marks as closed or removes from DB)
 bool close_account(const char* account_no, const char* pin) {
     FILE* file = fopen(DB_FILENAME, "r");
     if (!file) return false;
     FILE* temp = fopen("temp.txt", "w");
     if (!temp) {
         fclose(file);
         return false;
     }
     char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
     bool closed = false;
     lock_file(file, true);
     lock_file(temp, true);
     while (fgets(line, sizeof(line), file)) {
         if (sscanf(line, "%s %s", file_acct, file_pin) == 2) {
             if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0) {
                 closed = true;
                 continue;
             }
         }
         fputs(line, temp);
     }
     unlock_file(file);
     unlock_file(temp);
     fclose(file);
     fclose(temp);
     remove(DB_FILENAME);
     rename("temp.txt", DB_FILENAME);
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
     if (!update_balance(account_no, balance - amount)) return false;
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
     if (!update_balance(account_no, balance + amount)) return false;
     log_transaction(account_no, "DEPOSIT", amount, balance + amount);
     return true;
 }
 
 // Returns the balance in the account
 bool balance(const char* account_no, double* out_balance) {
     FILE* file = fopen(DB_FILENAME, "r");
     if (!file) return false;
     char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN];
     double file_balance;
     bool found = false;
     while (fgets(line, sizeof(line), file)) {
         // Skip to the 6th field (balance)
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
     FILE *db = fopen(DB_FILENAME, "r");
     if (!db) return false;
     char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
     bool found = false;
     while (fgets(line, sizeof(line), db))
         if (sscanf(line, "%s %s", file_acct, file_pin) == 2 &&
             !strcmp(file_acct, account_no) && !strcmp(file_pin, pin)) { found = true; break; }
     fclose(db);
     if (!found) return false;
     char filename[64]; snprintf(filename, sizeof(filename), "%s.txt", account_no);
     FILE *tf = fopen(filename, "r"); if (!tf) return false;
     char all[5][MAX_LINE_LEN] = {{0}}; int count = 0;
     while (fgets(line, sizeof(line), tf))
         count < 5 ? strncpy(all[count++], line, MAX_LINE_LEN) :
         (memmove(all, all+1, 4*MAX_LINE_LEN), strncpy(all[4], line, MAX_LINE_LEN));
     fclose(tf);
     for (int i = 0; i < 5; ++i) strncpy(transactions[i], all[i], MAX_LINE_LEN);
     return count > 0;
 }
 
 void log_transaction(const char* account_no, const char* type, double amount, double balance) {
     char filename[64]; snprintf(filename, sizeof(filename), "%s.txt", account_no);
     FILE* file = fopen(filename, "a"); if (!file) return;
     time_t now = time(NULL); char timebuf[32];
     strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
     fprintf(file, "%s %.2f %.2f %s\n", type, amount, balance, timebuf);
     fclose(file);
 }