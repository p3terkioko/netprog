#include "common.h"
#include <sys/file.h>
#include <dirent.h>

// Check if account exists
bool account_exists(const char* account_no) {
    FILE* file = fopen(DB_FILENAME, "rb");
    if (!file) {
        return false;
    }

    Account account;
    while (fread(&account, sizeof(Account), 1, file) == 1) {
        if (strcmp(account.account_no, account_no) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

// Get account details
bool get_account(const char* account_no, Account* account) {
    FILE* file = fopen(DB_FILENAME, "rb");
    if (!file) {
        return false;
    }

    while (fread(account, sizeof(Account), 1, file) == 1) {
        if (strcmp(account->account_no, account_no) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

// Add a new account
bool add_account(const Account* account) {
    FILE* file = fopen(DB_FILENAME, "ab");
    if (!file) {
        return false;
    }

    if (fwrite(account, sizeof(Account), 1, file) != 1) {
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

// Update account details
bool update_account(const Account* account) {
    FILE* file = fopen(DB_FILENAME, "rb+");
    if (!file) {
        return false;
    }

    Account temp_account;
    while (fread(&temp_account, sizeof(Account), 1, file) == 1) {
        if (strcmp(temp_account.account_no, account->account_no) == 0) {
            fseek(file, -sizeof(Account), SEEK_CUR);
            if (fwrite(account, sizeof(Account), 1, file) != 1) {
                fclose(file);
                return false;
            }
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

// Delete an account
bool delete_account(const char* account_no) {
    FILE* file = fopen(DB_FILENAME, "rb");
    FILE* temp_file = fopen("temp.dat", "wb");
    if (!file || !temp_file) {
        if (file) fclose(file);
        if (temp_file) fclose(temp_file);
        return false;
    }

    Account account;
    bool found = false;
    while (fread(&account, sizeof(Account), 1, file) == 1) {
        if (strcmp(account.account_no, account_no) != 0) {
            fwrite(&account, sizeof(Account), 1, temp_file);
        } else {
            found = true;
        }
    }

    fclose(file);
    fclose(temp_file);

    remove(DB_FILENAME);
    rename("temp.dat", DB_FILENAME);

    return found;
}

// Log a transaction
void log_transaction(const char* account_no, const char* type, double amount, double balance) {
    FILE* log_file = fopen(TRANSACTION_LOG_DIR "/transactions.log", "a");
    if (!log_file) {
        perror("Unable to open transaction log file");
        return;
    }

    time_t now = time(NULL);
    char* timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';  // Remove the newline character

    fprintf(log_file, "%s | Account: %s | Type: %s | Amount: %.2lf | Balance: %.2lf\n",
            timestamp, account_no, type, amount, balance);

    fclose(log_file);
}

// Get account transactions
bool get_transactions(const char* account_no, char transactions[][MAX_LINE_LEN], int max_transactions) {
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename), "%s/transactions.log", TRANSACTION_LOG_DIR);

    FILE* log_file = fopen(log_filename, "r");
    if (!log_file) {
        return false;
    }

    char line[MAX_LINE_LEN];
    int count = 0;

    while (fgets(line, sizeof(line), log_file)) {
        if (strstr(line, account_no)) {
            strncpy(transactions[count], line, MAX_LINE_LEN - 1);
            transactions[count][MAX_LINE_LEN - 1] = '\0';
            count++;
            if (count >= max_transactions) {
                break;
            }
        }
    }

    fclose(log_file);
    return count > 0;
}

// Generate a 4-digit PIN
void generate_pin(char* pin) {
    srand(time(NULL) + getpid());
    for (int i = 0; i < 4; i++) {
        pin[i] = '0' + rand() % 10;
    }
    pin[4] = '\0';
}

// Validate a PIN
bool validate_pin(const char* account_no, const char* pin) {
    Account account;
    if (get_account(account_no, &account)) {
        return strcmp(account.pin, pin) == 0;
    }
    return false;
}

// Validate an amount
bool is_valid_amount(double amount) {
    return amount > 0;
}

// Trim newline from strings
void trim_newline(char* str) {
    char* newline = strchr(str, '\n');
    if (newline) {
        *newline = '\0';
    }
}
// Process a banking request
char* process_request(const char* request) {
    char operation[32];
    char account_no[MAX_ACCT_LEN];
    double amount = 0.0;
    char* response;
    Account account;

    if (!parse_message(request, operation, account_no, &amount)) {
        return strdup(RESP_INVALID_REQUEST);
    }

    if (strcmp(operation, OP_REGISTER) == 0) {
        // Register new account
        if (account_exists(account_no)) {
            return strdup(RESP_ACCT_EXISTS);
        }

        Account new_account;
        strncpy(new_account.account_no, account_no, MAX_ACCT_LEN);
        generate_pin(new_account.pin);
        new_account.balance = 0.0;
        
        if (add_account(&new_account)) {
            return strdup(RESP_OK);
        } else {
            return strdup(RESP_ERROR);
        }
    }
    else if (strcmp(operation, OP_DEPOSIT) == 0) {
        // Deposit money
        if (!account_exists(account_no)) {
            return strdup(RESP_ACCT_NOT_FOUND);
        }

        if (!is_valid_amount(amount)) {
            return strdup(RESP_INVALID_AMOUNT);
        }

        if (get_account(account_no, &account)) {
            account.balance += amount;
            if (update_account(&account)) {
                log_transaction(account_no, "DEPOSIT", amount, account.balance);
                return create_response(RESP_OK, account.balance);
            }
        }
        return strdup(RESP_ERROR);
    }
    else if (strcmp(operation, OP_WITHDRAW) == 0) {
        // Withdraw money
        if (!account_exists(account_no)) {
            return strdup(RESP_ACCT_NOT_FOUND);
        }

        if (!is_valid_amount(amount)) {
            return strdup(RESP_INVALID_AMOUNT);
        }

        if (get_account(account_no, &account)) {
            if (account.balance < amount) {
                return strdup(RESP_INSUFFICIENT_FUNDS);
            }
            account.balance -= amount;
            if (update_account(&account)) {
                log_transaction(account_no, "WITHDRAW", amount, account.balance);
                return create_response(RESP_OK, account.balance);
            }
        }
        return strdup(RESP_ERROR);
    }
    else if (strcmp(operation, OP_CHECK) == 0) {
        // Check balance
        if (!account_exists(account_no)) {
            return strdup(RESP_ACCT_NOT_FOUND);
        }

        if (get_account(account_no, &account)) {
            return create_response(RESP_OK, account.balance);
        }
        return strdup(RESP_ERROR);
    }

    return strdup(RESP_INVALID_REQUEST);
}
// Create a message string
char* create_message(const char* operation, const char* account_no, double amount) {
    char* message = malloc(MAX_MSG_LEN);
    if (amount > 0) {
        snprintf(message, MAX_MSG_LEN, "%s %s %.2lf", operation, account_no, amount);
    } else {
        snprintf(message, MAX_MSG_LEN, "%s %s", operation, account_no);
    }
    return message;
}

// Parse a message string
bool parse_message(const char* message, char* operation, char* account_no, double* amount) {
    return sscanf(message, "%31s %15s %lf", operation, account_no, amount) >= 2;
}

// Create a response string
char* create_response(const char* status, double balance) {
    char* response = malloc(MAX_MSG_LEN);
    snprintf(response, MAX_MSG_LEN, "%s %.2lf", status, balance);
    return response;
}

// Parse a response string
bool parse_response(const char* response, char* status, double* balance) {
    return sscanf(response, "%31s %lf", status, balance) >= 1;
}