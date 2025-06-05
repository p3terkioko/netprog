#define _GNU_SOURCE // Must be first
#include "common.h"
#include <stdio.h>  // For fileno, fopen, etc.
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>      // For close, read, write, etc.
#include <sys/socket.h>  // For socket APIs
#include <netinet/in.h>  // For sockaddr_in
#include <arpa/inet.h>   // For inet_pton, etc.
#include <sys/file.h>    // For flock
#include <fcntl.h>       // For fcntl

#define DB_FILENAME "data.txt"
#define MAX_ARGS 5       // Max arguments for parse_message

// Forward declarations (prototypes)
char* handle_client_operation(const char* received_message);
static void log_transaction(const char* account_no, const char* type, double amount, double balance_after);
static char* create_response(const char* status, const char* arg1, const char* arg2);
static bool parse_message(const char* message, char* operation, char args[MAX_ARGS][MAX_LINE_LEN], int* arg_count);
static void trim(char* str);
// Note: is_valid_account_no is not static in the current file, so no prototype here unless changed.
// bool is_valid_account_no(const char* account_no);
// bool is_valid_amount_str(const char* amount_str); // Not static either
static void generate_pin(char* pin_out);
static void generate_account_no(char* acct_out);
static bool verify_pin(const char* account_no, const char* pin); // Already had this one
static bool account_exists(const char* account_no);
static bool get_balance_internal(const char* account_no, double* balance);
static bool update_balance(const char* account_no, double new_balance);
static bool add_account_record(const char* account_no, const char* pin, const char* name, const char* national_id, const char* account_type, double initial_deposit);
static void lock_file(FILE* file, bool exclusive);
static void unlock_file(FILE* file);
// bool is_valid_amount(double amount); // Also not static

// Utility Functions (Copied and some made static)
static void trim(char* str) {
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
    // Adjusted to use MAX_ACCT_LEN from common.h for consistency
    if (strlen(account_no) > MAX_ACCT_LEN) return false;
    for (const char* p = account_no; *p; ++p) if (!isdigit((unsigned char)*p)) return false;
    return true;
}

// Changed to is_valid_amount_str to take string input, as atof is done later
bool is_valid_amount_str(const char* amount_str) {
    if (!amount_str || !*amount_str) return false;
    int dot_count = 0;
    for (const char* p = amount_str; *p; ++p) {
        if (*p == '.') {
            dot_count++;
            if (dot_count > 1) return false;
        } else if (!isdigit((unsigned char)*p)) {
            return false;
        }
    }
    // Further validation (e.g. minimum amount) is business logic dependent
    return true;
}

// Original is_valid_amount(double) can still be useful internally
bool is_valid_amount(double amount) {
    return amount > 0; // Basic check, specific rules (e.g. min deposit) handled by logic funcs
}


// File I/O Functions (Copied and some made static)
static void lock_file(FILE* file, bool exclusive) {
#ifdef _WIN32
    // Windows file locking - Placeholder, not fully implemented/tested for this project
    // HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    // OVERLAPPED overlapped = {0};
    // if (exclusive) {
    //     LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlapped);
    // } else {
    //     LockFileEx(hFile, 0, 0, MAXDWORD, MAXDWORD, &overlapped);
    // }
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
    // HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    // OVERLAPPED overlapped = {0};
    // UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
#else
    flock(fileno(file), LOCK_UN);
#endif
}

// account_exists checks if an account number is present in the DB_FILENAME
// This version doesn't check PIN, just existence.
bool account_exists(const char* account_no) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        return false; // File doesn't exist, so account cannot exist
    }
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN + 1]; // +1 for null terminator
    bool found = false;

    lock_file(file, false); // Lock for shared reading
    while (fgets(line, sizeof(line), file)) {
        // sscanf should parse account number, pin, name, national_id, type, balance
        if (sscanf(line, "%s %*s %*s %*s %*s %*f", file_acct) == 1) {
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


// get_balance: Retrieves balance for an account_no. PIN check is now responsibility of caller or higher-level func.
// For server-side internal use where PIN might have been verified already.
// The public 'balance' function for client requests will handle PIN.
static bool get_balance_internal(const char* account_no, double* balance_out) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) return false;

    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN + 1];
    char name_buf[64], nid_buf[32], type_buf[16], pin_buf[10]; // Match bankv1.c sizes
    double current_balance;
    bool found = false;

    lock_file(file, false);
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, pin_buf, name_buf, nid_buf, type_buf, &current_balance) == 6) {
            if (strcmp(file_acct, account_no) == 0) {
                *balance_out = current_balance;
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
        perror("update_balance: DB_FILENAME read error");
        return false;
    }
    // Use a temporary file for atomic update
    char temp_db_filename[20];
    snprintf(temp_db_filename, sizeof(temp_db_filename), "temp_%d.txt", rand());

    FILE* temp_file = fopen(temp_db_filename, "w");
    if (!temp_file) {
        perror("update_balance: temp_file create error");
        fclose(file);
        return false;
    }

    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN + 1];
    char file_pin[10], name[64], national_id[32], account_type[16]; // Increased sizes
    double current_balance;
    bool updated = false;

    lock_file(file, true); // Exclusive lock for read and subsequent replace
    lock_file(temp_file, true);

    while (fgets(line, sizeof(line), file)) {
        // Read all fields to write them back correctly
        if (sscanf(line, "%s %s %63s %31s %15s %lf", file_acct, file_pin, name, national_id, account_type, &current_balance) == 6) {
            if (strcmp(file_acct, account_no) == 0) {
                fprintf(temp_file, "%s %s %s %s %s %.2f\n", file_acct, file_pin, name, national_id, account_type, new_balance);
                updated = true;
            } else {
                fputs(line, temp_file); // Copy non-matching lines as is
            }
        } else {
             fputs(line, temp_file); // Copy lines that don't match format (e.g. corrupted, or headers if any)
        }
    }

    unlock_file(temp_file);
    fclose(temp_file);
    unlock_file(file);
    fclose(file);

    if (updated) {
        if (remove(DB_FILENAME) != 0) {
            perror("update_balance: Error removing original DB_FILENAME");
            remove(temp_db_filename); // Clean up temp file
            return false;
        }
        if (rename(temp_db_filename, DB_FILENAME) != 0) {
            perror("update_balance: Error renaming temp_file to DB_FILENAME");
            // Attempt to restore original if possible, or log critical error
            return false;
        }
    } else {
        remove(temp_db_filename); // No update was made, remove temp file
    }
    return updated;
}

// add_account is used by open_account. It doesn't check for existence, assumes caller does.
// This is the low-level add. open_account is the business logic.
static bool add_account_record(const char* account_no, const char* pin, const char* name, const char* national_id, const char* account_type, double initial_deposit) {
    FILE* file = fopen(DB_FILENAME, "a"); // Append mode
    if (!file) {
        perror("add_account_record: Error opening DB_FILENAME for append");
        return false;
    }
    lock_file(file, true); // Exclusive lock for appending
    fprintf(file, "%s %s %s %s %s %.2f\n", account_no, pin, name, national_id, account_type, initial_deposit);
    fflush(file); // Ensure data is written to disk
    unlock_file(file);
    fclose(file);
    return true;
}

// Helper to generate a random 4-digit PIN
static void generate_pin(char* pin_out) {
    // More robust seeding if server runs for long time
    // srand((unsigned int)time(NULL) ^ (unsigned int)getpid() ^ (unsigned int)pthread_self()); // If using pthreads
    srand((unsigned int)time(NULL) ^ (unsigned int)rand()); // Simplified for now
    sprintf(pin_out, "%04d", 1000 + rand() % 9000); // Generate 4-digit PIN
}

// Helper to generate a unique account number (simple incremental)
static void generate_account_no(char* acct_out) {
    FILE* file = fopen(DB_FILENAME, "r");
    int max_acct_val = 100000; // Starting base account number
    char line[MAX_LINE_LEN];
    char file_acct_str[MAX_ACCT_LEN + 1];

    if (file) {
        lock_file(file, false);
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "%s %*s %*s %*s %*s %*f", file_acct_str) == 1) {
                int current_acct_val = atoi(file_acct_str);
                if (current_acct_val > max_acct_val) {
                    max_acct_val = current_acct_val;
                }
            }
        }
        unlock_file(file);
        fclose(file);
    }
    sprintf(acct_out, "%d", max_acct_val + 1);
}


bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit) {
    if (!name || !national_id || !account_type || !out_account_no || !out_pin) return false;
    if (strlen(name) == 0 || strlen(national_id) == 0 || strlen(account_type) == 0) return false;

    if (initial_deposit < 1000.0) { // Minimum initial deposit
        fprintf(stderr, "Initial deposit must be at least 1000.00\n");
        return false;
    }
    if (strcmp(account_type, "savings") != 0 && strcmp(account_type, "checking") != 0) {
        fprintf(stderr, "Invalid account type. Must be 'savings' or 'checking'.\n");
        return false;
    }

    generate_account_no(out_account_no);
    generate_pin(out_pin);

    // It's possible (though unlikely with incremental account numbers) that a generated account_no could collide
    // if records were manually deleted or if generation logic changes. A robust system might re-check existence here.
    // For this version, we assume generate_account_no provides a unique number.

    if (add_account_record(out_account_no, out_pin, name, national_id, account_type, initial_deposit)) {
        log_transaction(out_account_no, "OPEN_ACCOUNT", initial_deposit, initial_deposit);
        return true;
    }
    return false;
}

bool close_account(const char* account_no, const char* pin) {
    if (!is_valid_account_no(account_no) || !pin || strlen(pin) == 0) return false;

    if (!verify_pin(account_no, pin)) {
        fprintf(stderr, "Close account failed: Invalid PIN for %s\n", account_no);
        return false;
    }

    // Check balance - some banks might require $0 balance before closing
    double current_bal;
    if(get_balance_internal(account_no, &current_bal) && current_bal != 0.0){
        // For now, allow closing with balance. Money would be "lost" or need manual handling.
        // A real system would enforce $0 balance or transfer funds.
        fprintf(stdout, "Warning: Closing account %s with non-zero balance: %.2f\n", account_no, current_bal);
    }


    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        perror("close_account: DB_FILENAME read error");
        return false;
    }
    char temp_db_filename[20];
    snprintf(temp_db_filename, sizeof(temp_db_filename), "temp_%d.txt", rand());
    FILE* temp_file = fopen(temp_db_filename, "w");
    if (!temp_file) {
        perror("close_account: temp_file create error");
        fclose(file);
        return false;
    }

    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN + 1], file_pin_stored[10];
    bool closed = false;

    lock_file(file, true);
    lock_file(temp_file, true);

    while (fgets(line, sizeof(line), file)) {
        // sscanf only needs to check account_no and pin for the decision to exclude.
        if (sscanf(line, "%s %s %*s %*s %*s %*f", file_acct, file_pin_stored) == 2) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin_stored, pin) == 0) {
                closed = true; // Found the account, skip writing it to temp_file (effectively deleting it)
                continue;
            }
        }
        fputs(line, temp_file); // Write other lines to temp_file
    }

    unlock_file(temp_file);
    fclose(temp_file);
    unlock_file(file);
    fclose(file);

    if (closed) {
        if (remove(DB_FILENAME) != 0) {
            perror("close_account: Error removing original DB_FILENAME");
            remove(temp_db_filename);
            return false;
        }
        if (rename(temp_db_filename, DB_FILENAME) != 0) {
            perror("close_account: Error renaming temp_file to DB_FILENAME");
            return false;
        }
        // Also remove the transaction log file for the closed account
        char transaction_log_filename[MAX_ACCT_LEN + 4 + 1]; // account_no + ".txt" + null
        snprintf(transaction_log_filename, sizeof(transaction_log_filename), "%s.txt", account_no);
        if (remove(transaction_log_filename) != 0) {
            perror("close_account: Error removing transaction log file");
            // This is not critical for account closure itself, so don't return false. Log it.
            fprintf(stderr, "Warning: Could not remove transaction log %s for closed account %s\n", transaction_log_filename, account_no);
        }
        log_transaction(account_no, "CLOSE_ACCOUNT", 0, 0); // Log closure
    } else {
        remove(temp_db_filename); // Account not found or PIN incorrect, remove temp file
    }
    return closed;
}


bool deposit_extended(const char* account_no, const char* pin, double amount) {
    if (!is_valid_account_no(account_no) || !pin || strlen(pin) == 0) return false;
    if (amount < 500.0) { // Minimum deposit amount
        fprintf(stderr, "Deposit amount must be at least 500.00\n");
        return false;
    }
    if (!is_valid_amount(amount)) { // General validity (e.g. >0)
         fprintf(stderr, "Invalid deposit amount.\n");
        return false;
    }

    if (!verify_pin(account_no, pin)) {
        fprintf(stderr, "Deposit failed: Invalid PIN for %s\n", account_no);
        return false;
    }

    double current_balance;
    if (!get_balance_internal(account_no, &current_balance)) {
        fprintf(stderr, "Deposit failed: Could not retrieve balance for %s (account may not exist post-PIN check, data integrity issue)\n", account_no);
        return false; // Should not happen if verify_pin passed and data is consistent
    }

    double new_balance = current_balance + amount;
    if (update_balance(account_no, new_balance)) {
        log_transaction(account_no, "DEPOSIT", amount, new_balance);
        return true;
    }
    fprintf(stderr, "Deposit failed: Could not update balance for %s\n", account_no);
    return false;
}

bool withdraw_extended(const char* account_no, const char* pin, double amount) {
    if (!is_valid_account_no(account_no) || !pin || strlen(pin) == 0) return false;
    if (amount < 500.0) { // Minimum withdrawal amount
        fprintf(stderr, "Withdrawal amount must be at least 500.00\n");
        return false;
    }
     if (!is_valid_amount(amount)) {
         fprintf(stderr, "Invalid withdrawal amount.\n");
        return false;
    }

    if (!verify_pin(account_no, pin)) {
        fprintf(stderr, "Withdrawal failed: Invalid PIN for %s\n", account_no);
        return false;
    }

    double current_balance;
    if (!get_balance_internal(account_no, &current_balance)) {
        fprintf(stderr, "Withdrawal failed: Could not retrieve balance for %s\n", account_no);
        return false;
    }

    if (current_balance - amount < 1000.0) { // Minimum balance to maintain
        fprintf(stderr, "Withdrawal failed: Insufficient funds or would fall below minimum balance for %s\n", account_no);
        return false;
    }

    double new_balance = current_balance - amount;
    if (update_balance(account_no, new_balance)) {
        log_transaction(account_no, "WITHDRAW", amount, new_balance);
        return true;
    }
    fprintf(stderr, "Withdrawal failed: Could not update balance for %s\n", account_no);
    return false;
}

// 'balance' function for client requests - includes PIN check.
bool balance(const char* account_no, const char* pin, double* out_balance) {
    if (!is_valid_account_no(account_no) || !pin || strlen(pin) == 0 || !out_balance) return false;

    if (!verify_pin(account_no, pin)) {
        fprintf(stderr, "Balance check failed: Invalid PIN for %s\n", account_no);
        return false;
    }
    return get_balance_internal(account_no, out_balance);
}


bool statement(const char* account_no, const char* pin, char transactions_out[5][MAX_LINE_LEN]) {
    if (!is_valid_account_no(account_no) || !pin || strlen(pin) == 0 || !transactions_out) return false;

    for(int i=0; i<5; ++i) transactions_out[i][0] = '\0'; // Initialize output array

    if (!verify_pin(account_no, pin)) {
        fprintf(stderr, "Statement retrieval failed: Invalid PIN for %s\n", account_no);
        return false;
    }

    char transaction_log_filename[MAX_ACCT_LEN + 4 + 1]; // account_no + ".txt" + null
    snprintf(transaction_log_filename, sizeof(transaction_log_filename), "%s.txt", account_no);

    FILE* tf = fopen(transaction_log_filename, "r");
    if (!tf) {
        // It's not an error if the log file doesn't exist (e.g., no transactions yet).
        // The client will receive an empty statement for RESP_OK.
        return true; // No transactions, but PIN was valid and account exists.
    }

    char all_lines_buffer[5][MAX_LINE_LEN] = {{0}};
    int line_count = 0;
    char current_line[MAX_LINE_LEN];

    lock_file(tf, false);
    while (fgets(current_line, sizeof(current_line), tf)) {
        // Remove newline characters from the line
        char* nl = strchr(current_line, '\n');
        if (nl) *nl = '\0';
        nl = strchr(current_line, '\r'); // Also check for CR
        if (nl) *nl = '\0';

        if (line_count < 5) {
            strncpy(all_lines_buffer[line_count], current_line, MAX_LINE_LEN - 1);
            all_lines_buffer[line_count][MAX_LINE_LEN - 1] = '\0';
        } else {
            // Shift lines up to make space for the new line at the end
            for (int i = 0; i < 4; ++i) {
                strncpy(all_lines_buffer[i], all_lines_buffer[i+1], MAX_LINE_LEN -1);
                 all_lines_buffer[i][MAX_LINE_LEN - 1] = '\0';
            }
            strncpy(all_lines_buffer[4], current_line, MAX_LINE_LEN - 1);
            all_lines_buffer[4][MAX_LINE_LEN - 1] = '\0';
        }
        line_count++;
    }
    unlock_file(tf);
    fclose(tf);

    // Copy the last 5 (or fewer) lines to transactions_out
    // int start_index = (line_count < 5) ? 0 : line_count - 5; // This variable is unused.
    int output_idx = 0;
    for (int i = 0; i < 5; ++i) { // Iterate through the buffer which contains last 5 lines
        if (strlen(all_lines_buffer[i]) > 0) {
             strncpy(transactions_out[output_idx++], all_lines_buffer[i], MAX_LINE_LEN -1);
             transactions_out[output_idx-1][MAX_LINE_LEN-1] = '\0';
        }
    }
    return true; // Successfully retrieved transactions (or confirmed none exist)
}

void log_transaction(const char* account_no, const char* type, double amount, double balance_after) {
    if (!is_valid_account_no(account_no)) return;

    char transaction_log_filename[MAX_ACCT_LEN + 4 + 1];
    snprintf(transaction_log_filename, sizeof(transaction_log_filename), "%s.txt", account_no);

    FILE* file = fopen(transaction_log_filename, "a");
    if (!file) {
        perror("log_transaction: Error opening transaction log file");
        return;
    }

    time_t now = time(NULL);
    char time_buffer[32];
    // Format time as YYYY-MM-DD HH:MM:SS
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

    lock_file(file, true);
    fprintf(file, "%s: %s, Amt: %.2f, Bal: %.2f\n", time_buffer, type, amount, balance_after);
    fflush(file);
    unlock_file(file);
    fclose(file);
}

// Simplified register_account, deposit, withdraw, check_balance from original bankv1.c
// These are mostly superseded by *_extended versions or direct logic in handle_client_operation.
// They are kept here for reference or potential internal use if adapted.
// Note: These do not perform PIN checks and use simpler file I/O.
char* register_account_simple(const char* account_no) {
    // This function is too simple; open_account is the primary way.
    // If used, it should integrate with the full data structure.
    // For now, effectively deprecated by open_account.
    return create_response(RESP_ERROR, "Simple registration disabled", NULL);
}
// ... (deposit_simple, withdraw_simple, check_balance_simple could be here if needed)


// Message Parsing and Response Creation
bool parse_message(const char* message, char* operation, char args[MAX_ARGS][MAX_LINE_LEN], int* arg_count) {
    char buffer[MAX_MSG_LEN];
    strncpy(buffer, message, MAX_MSG_LEN - 1);
    buffer[MAX_MSG_LEN - 1] = '\0';
    trim(buffer); // Trim leading/trailing whitespace from the whole message

    *arg_count = 0;
    memset(operation, 0, MAX_LINE_LEN);
    for(int i=0; i<MAX_ARGS; ++i) memset(args[i], 0, MAX_LINE_LEN);

    char* token = strtok(buffer, " ");
    if (!token) return false; // No operation found
    strncpy(operation, token, MAX_LINE_LEN - 1);
    operation[MAX_LINE_LEN -1] = '\0'; // Ensure null termination

    while ((token = strtok(NULL, " ")) != NULL && *arg_count < MAX_ARGS) {
        strncpy(args[*arg_count], token, MAX_LINE_LEN - 1);
        args[*arg_count][MAX_LINE_LEN -1] = '\0'; // Ensure null termination
        (*arg_count)++;
    }
    return true;
}

char* create_response(const char* status, const char* arg1, const char* arg2) {
    static char response_buffer[MAX_MSG_LEN];
    memset(response_buffer, 0, MAX_MSG_LEN);

    if (!status) return ""; // Should not happen

    strncat(response_buffer, status, MAX_MSG_LEN - strlen(response_buffer) - 1);
    if (arg1) {
        strncat(response_buffer, " ", MAX_MSG_LEN - strlen(response_buffer) - 1);
        strncat(response_buffer, arg1, MAX_MSG_LEN - strlen(response_buffer) - 1);
    }
    if (arg2) {
        strncat(response_buffer, " ", MAX_MSG_LEN - strlen(response_buffer) - 1);
        strncat(response_buffer, arg2, MAX_MSG_LEN - strlen(response_buffer) - 1);
    }
    return response_buffer;
}

// Helper function to verify PIN against stored PIN
static bool verify_pin(const char* account_no, const char* pin_attempt) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) {
        perror("verify_pin: DB_FILENAME read error");
        return false;
    }
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN + 1], file_pin_stored[10]; // Increased pin buffer
    bool pin_ok = false;

    lock_file(file, false);
    while (fgets(line, sizeof(line), file)) {
        // sscanf needs to parse account_no and pin_stored
        if (sscanf(line, "%s %s %*s %*s %*s %*f", file_acct, file_pin_stored) == 2) {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin_stored, pin_attempt) == 0) {
                pin_ok = true;
                break;
            }
        }
    }
    unlock_file(file);
    fclose(file);
    return pin_ok;
}


// Main Request Handler
char* handle_client_operation(const char* received_message) {
    char operation[MAX_LINE_LEN];
    char args[MAX_ARGS][MAX_LINE_LEN];
    int arg_count;

    if (!parse_message(received_message, operation, args, &arg_count)) {
        return create_response(RESP_INVALID_REQUEST, "Malformed message format", NULL);
    }

    if (strcmp(operation, OP_OPEN_ACCOUNT) == 0) {
        if (arg_count < 4) return create_response(RESP_INVALID_REQUEST, "Too few arguments for OPEN_ACCOUNT", NULL);
        // args[0]=name, args[1]=national_id, args[2]=account_type, args[3]=initial_deposit_str
        if (!is_valid_amount_str(args[3])) return create_response(RESP_INVALID_AMOUNT, "Invalid deposit amount format", NULL);
        double initial_deposit = atof(args[3]);
        char new_acct_no[MAX_ACCT_LEN + 1];
        char new_pin[10];

        if (open_account(args[0], args[1], args[2], new_acct_no, new_pin, initial_deposit)) {
            return create_response(RESP_OK, new_acct_no, new_pin);
        } else {
            // More specific error could be determined from open_account if it set errno or returned error codes
            return create_response(RESP_ERROR, "Account opening failed", NULL);
        }
    } else if (strcmp(operation, OP_DEPOSIT) == 0) {
        if (arg_count < 3) return create_response(RESP_INVALID_REQUEST, "Too few arguments for DEPOSIT", NULL);
        // args[0]=account_no, args[1]=pin, args[2]=amount_str
        if (!is_valid_account_no(args[0])) return create_response(RESP_INVALID_REQUEST, "Invalid account number format", NULL);
        if (!is_valid_amount_str(args[2])) return create_response(RESP_INVALID_AMOUNT, "Invalid deposit amount format", NULL);
        double amount = atof(args[2]);

        if (deposit_extended(args[0], args[1], amount)) {
            double current_balance; // Fetch current balance to return
            if (balance(args[0], args[1], &current_balance)) { // balance function now checks PIN
                 char balance_str[MAX_AMT_LEN + 1];
                 snprintf(balance_str, sizeof(balance_str), "%.2f", current_balance);
                 return create_response(RESP_OK, balance_str, "Deposit successful");
            } else {
                 // This case should ideally not be reached if deposit_extended succeeded and data is consistent
                 return create_response(RESP_OK, "Deposited", "Balance unavailable post-deposit");
            }
        } else {
            // deposit_extended logs details. Check for specific error like invalid PIN, insufficient funds etc.
            // For now, generic error.
            return create_response(RESP_ERROR, "Deposit operation failed", NULL);
        }
    } else if (strcmp(operation, OP_WITHDRAW) == 0) {
        if (arg_count < 3) return create_response(RESP_INVALID_REQUEST, "Too few arguments for WITHDRAW", NULL);
        // args[0]=account_no, args[1]=pin, args[2]=amount_str
        if (!is_valid_account_no(args[0])) return create_response(RESP_INVALID_REQUEST, "Invalid account number format", NULL);
        if (!is_valid_amount_str(args[2])) return create_response(RESP_INVALID_AMOUNT, "Invalid withdrawal amount format", NULL);
        double amount = atof(args[2]);

        if (withdraw_extended(args[0], args[1], amount)) {
            double current_balance;
            if (balance(args[0], args[1], &current_balance)) {
                char balance_str[MAX_AMT_LEN + 1];
                snprintf(balance_str, sizeof(balance_str), "%.2f", current_balance);
                return create_response(RESP_OK, balance_str, "Withdrawal successful");
            } else {
                return create_response(RESP_OK, "Withdrawn", "Balance unavailable post-withdrawal");
            }
        } else {
            // Check if it was insufficient funds, invalid PIN, etc.
            // For now, a generic error. withdraw_extended logs specifics.
            // A more specific error might be RESP_INSUFFICIENT_FUNDS
            return create_response(RESP_ERROR, "Withdrawal operation failed", NULL);
        }
    } else if (strcmp(operation, OP_CHECK) == 0) { // Renamed from OP_CHECK_BALANCE for client
         if (arg_count < 2) return create_response(RESP_INVALID_REQUEST, "Too few arguments for CHECK_BALANCE", NULL);
         // args[0]=account_no, args[1]=pin
         if (!is_valid_account_no(args[0])) return create_response(RESP_INVALID_REQUEST, "Invalid account number format", NULL);

         double current_balance;
         if (balance(args[0], args[1], &current_balance)) { // balance function handles PIN check
             char balance_str[MAX_AMT_LEN + 1];
             snprintf(balance_str, sizeof(balance_str), "%.2f", current_balance);
             return create_response(RESP_OK, balance_str, NULL);
         } else {
             // balance function would have failed due to non-existent account or bad PIN
             return create_response(RESP_ACCT_NOT_FOUND, "Account not found or PIN incorrect", NULL);
         }
    } else if (strcmp(operation, OP_STATEMENT) == 0) {
        if (arg_count < 2) return create_response(RESP_INVALID_REQUEST, "Too few arguments for STATEMENT", NULL);
        // args[0]=account_no, args[1]=pin
        if (!is_valid_account_no(args[0])) return create_response(RESP_INVALID_REQUEST, "Invalid account number format", NULL);

        char transactions[5][MAX_LINE_LEN]; // As defined in common.h (implicitly) or bankv1.c
        for(int i=0; i<5; ++i) transactions[i][0] = '\0';

        if (statement(args[0], args[1], transactions)) { // statement function handles PIN check
            char statement_data_str[MAX_MSG_LEN] = ""; // Buffer for semicolon-separated transactions
            bool first_txn = true;
            for (int i = 0; i < 5; ++i) {
                if (strlen(transactions[i]) > 0) {
                    if (!first_txn) {
                        strncat(statement_data_str, ";", MAX_MSG_LEN - strlen(statement_data_str) - 1);
                    }
                    // Ensure transaction itself doesn't contain chars that break parsing for client or message structure
                    // For now, assume transactions are simple strings.
                    strncat(statement_data_str, transactions[i], MAX_MSG_LEN - strlen(statement_data_str) - 1);
                    first_txn = false;
                }
            }
            return create_response(RESP_OK, statement_data_str, NULL);
        } else {
            // statement failed, likely due to PIN or account not found
            return create_response(RESP_ERROR, "Could not retrieve statement (check account/PIN)", NULL);
        }
    } else if (strcmp(operation, OP_CLOSE_ACCOUNT) == 0) {
        if (arg_count < 2) return create_response(RESP_INVALID_REQUEST, "Too few arguments for CLOSE_ACCOUNT", NULL);
        // args[0]=account_no, args[1]=pin
        if (!is_valid_account_no(args[0])) return create_response(RESP_INVALID_REQUEST, "Invalid account number format", NULL);

        if (close_account(args[0], args[1])) { // close_account handles PIN check
            return create_response(RESP_OK, "Account closed successfully", NULL);
        } else {
            return create_response(RESP_ERROR, "Account closure failed (check account/PIN or balance)", NULL);
        }
    }
    else {
        return create_response(RESP_INVALID_REQUEST, "Unknown operation code", NULL);
    }
}


// Main Server Function
int main(int argc, char *argv[]) {
    int listen_fd;
    struct sockaddr_in server_addr;
    int port = SERVER_PORT; // Default port from common.h

    // Seed random number generator (used for PINs, temp filenames)
    // A more robust seed might involve getpid() or other sources of entropy,
    // but time(NULL) is common for simplicity.
    srand((unsigned int)time(NULL));


    if (argc == 2) { // User provided a port number
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s. Must be between 1 and 65535.\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else if (argc > 2) { // Too many arguments
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set socket option to reuse address, helpful for quick server restarts
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Set listening socket to non-blocking (required for epoll edge-triggered mode, good practice anyway)
    int flags = fcntl(listen_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed for listen_fd");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    if (fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK failed for listen_fd");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all available interfaces
    server_addr.sin_port = htons(port);              // Server port

    // Bind the socket to the server address and port
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(listen_fd, 10) == -1) { // Backlog of 10 pending connections
        perror("listen error");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);
    // Epoll setup
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("epoll_create1 failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS]; // MAX_EVENTS from common.h

    event.events = EPOLLIN | EPOLLET; // Edge-triggered for new connections on listen_fd
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        perror("epoll_ctl listen_fd failed");
        close(listen_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for connections on port %d using epoll...\n", port);

    char buffer[MAX_MSG_LEN]; // Reusable buffer for reads

    while(1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // Wait indefinitely
        if (num_events == -1) {
            if (errno == EINTR) continue; // Interrupted by signal, try again
            perror("epoll_wait failed");
            break; // Exit loop on other epoll_wait errors
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == listen_fd) {
                // New connection(s)
                while(1) { // Loop for ET on listen_fd
                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // All pending connections accepted for this ET notification
                            break;
                        } else {
                            perror("accept failed");
                            break; // Error accepting
                        }
                    }

                    // Set client socket to non-blocking
                    int flags_client = fcntl(client_fd, F_GETFL, 0);
                    if (flags_client == -1) {
                        perror("fcntl client F_GETFL");
                        close(client_fd);
                        continue; // Next connection attempt
                    }
                    if (fcntl(client_fd, F_SETFL, flags_client | O_NONBLOCK) == -1) {
                        perror("fcntl client F_SETFL O_NONBLOCK");
                        close(client_fd);
                        continue; // Next connection attempt
                    }

                    event.events = EPOLLIN | EPOLLET; // Edge-triggered for client data
                    event.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        perror("epoll_ctl add client_fd failed");
                        close(client_fd);
                    } else {
                        char client_ip_str[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, sizeof(client_ip_str));
                        printf("Accepted new connection from %s:%d on fd %d\n", client_ip_str, ntohs(client_addr.sin_port), client_fd);
                    }
                } // End accept loop
            } else {
                // Data from an existing client or error
                int current_fd = events[i].data.fd;

                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    fprintf(stderr, "Epoll error or HUP on fd %d. Closing connection.\n", current_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL); // Remove from epoll
                    close(current_fd);
                    continue;
                }

                if (events[i].events & EPOLLIN) {
                    ssize_t bytes_read_total = 0;
                    char client_message[MAX_MSG_LEN] = {0};

                    while (1) { // Loop for ET on client_fd read
                        ssize_t bytes_read = read(current_fd, buffer, sizeof(buffer) -1); // Leave space for null
                        if (bytes_read == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // No more data to read for now with non-blocking socket
                                break;
                            }
                            perror("read error on client_fd");
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            close(current_fd);
                            goto next_event_loop;
                        } else if (bytes_read == 0) {
                            printf("Client on fd %d disconnected.\n", current_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            close(current_fd);
                            goto next_event_loop;
                        }

                        buffer[bytes_read] = '\0'; // Null-terminate the chunk read
                        if (bytes_read_total + bytes_read < MAX_MSG_LEN) {
                            strcat(client_message, buffer); // Append to client_message
                            bytes_read_total += bytes_read;
                        } else {
                            fprintf(stderr, "Message from fd %d too long. Closing connection.\n", current_fd);
                            // Send error response if possible, then close
                            char* err_resp = create_response(RESP_ERROR, "Message too long", NULL);
                            send(current_fd, err_resp, strlen(err_resp), MSG_NOSIGNAL); // Try to send error
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            close(current_fd);
                            goto next_event_loop;
                        }
                        // Basic assumption: if read returns less than buffer size, it's end of current data for ET
                        // A more robust protocol would use delimiters (e.g. newline) or length prefixes
                        // For this task, we assume one logical message might be read in one or more 'read' calls in this loop
                        if (bytes_read < (ssize_t)(sizeof(buffer)-1) ) { // Heuristic for ET: read less than asked
                             // Check if the received data contains a logical end (e.g., newline)
                             // For simplicity, we process whatever we have accumulated if read would block or returned less than full
                             if (strchr(client_message, '\n') != NULL || bytes_read_total > 0) { // Process if newline or any data
                                break;
                             }
                        }
                    }


                    if (bytes_read_total > 0) {
                        client_message[bytes_read_total] = '\0';
                        trim(client_message);
                        printf("Received from fd %d: [%s]\n", current_fd, client_message);

                        char* response = handle_client_operation(client_message);
                        if (response) {
                            printf("Sending to fd %d: [%s]\n", current_fd, response);
                            ssize_t sent_total = 0;
                            ssize_t response_len = strlen(response);
                            while(sent_total < response_len) {
                                // MSG_NOSIGNAL prevents SIGPIPE if client closed connection prematurely
                                ssize_t bytes_sent = send(current_fd, response + sent_total, response_len - sent_total, MSG_NOSIGNAL);
                                if (bytes_sent == -1) {
                                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        fprintf(stdout, "Send would block for fd %d. Registering EPOLLOUT (basic).\n", current_fd);
                                        // Store remaining response and register EPOLLOUT (simplified)
                                        // For this subtask, we won't implement full buffer management for EPOLLOUT.
                                        // We'll just set the flag and expect EPOLLOUT to trigger.
                                        // A real implementation needs to store 'response + sent_total' and 'response_len - sent_total'.
                                        event.events = EPOLLIN | EPOLLOUT | EPOLLET; // Keep EPOLLIN for further client requests
                                        event.data.fd = current_fd;
                                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &event);
                                        // Store this FD and its pending message separately if not using client_conn_t struct
                                        // For now, this is a simplification.
                                        goto next_event_loop; // Don't try to send more now
                                    } else {
                                        perror("send error on client_fd");
                                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                                        close(current_fd);
                                        break;
                                    }
                                }
                                sent_total += bytes_sent;
                                if (sent_total >= response_len) break;
                            }
                        }
                    }
                } else if (events[i].events & EPOLLOUT) {
                    // This fd is ready for writing.
                    // This implies a previous send() blocked.
                    // A full implementation would retrieve the pending data for this fd and try to send it.
                    printf("EPOLLOUT triggered for fd %d. Attempting to complete send (simplified).\n", current_fd);
                    // For this simplified version, we don't have the stored message easily available here
                    // without a client connection struct. We will just unregister EPOLLOUT to prevent busy loop.
                    // In a real app: retrieve stored data for current_fd and continue sending.
                    // If all data sent, then: event.events = EPOLLIN | EPOLLET;
                    // If send blocks again: keep EPOLLOUT.
                    // If error: close fd.
                    event.events = EPOLLIN | EPOLLET; // Assume write completed or give up for this example
                    event.data.fd = current_fd;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &event) == -1){
                        perror("epoll_ctl EPOLLOUT -> EPOLLIN failed");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                        close(current_fd);
                    }
                }
            }
            next_event_loop:;
        }
    }

    // Cleanup
    printf("Server shutting down.\n");
    close(listen_fd);
    if (epoll_fd != -1) close(epoll_fd);

    return 0;
}
