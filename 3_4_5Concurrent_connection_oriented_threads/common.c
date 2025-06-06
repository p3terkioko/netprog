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