#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include "bank_utils.h"

// Cross-platform file locking function
static void lock_file(FILE *file, bool exclusive)
{
#ifdef _WIN32
    // Windows file locking
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    OVERLAPPED overlapped = {0};
    if (exclusive)
    {
        LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlapped);
    }
    else
    {
        LockFileEx(hFile, 0, 0, MAXDWORD, MAXDWORD, &overlapped);
    }
#else
    // Unix/Linux file locking
    if (exclusive)
    {
        flock(fileno(file), LOCK_EX);
    }
    else
    {
        flock(fileno(file), LOCK_SH);
    }
#endif
}

static void unlock_file(FILE *file)
{
#ifdef _WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    OVERLAPPED overlapped = {0};
    UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
#else
    flock(fileno(file), LOCK_UN);
#endif
}

void trim(char *str)
{
    if (!str)
        return;
    char *start = str;
    while (isspace((unsigned char)*start))
        start++;
    if (!*start)
    {
        *str = 0;
        return;
    }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;
    *(end + 1) = 0;
    if (start > str)
        memmove(str, start, strlen(start) + 1);
}

bool is_valid_account_no(const char *account_no)
{
    if (!account_no || !*account_no)
        return false;
    for (const char *p = account_no; *p; ++p)
        if (!isdigit(*p))
            return false;
    return true;
}

bool is_valid_amount(double amount) { return amount > 0; }

bool account_exists(const char *account_no)
{
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file)
    {
        return false;
    }
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN];
    bool found = false;
    lock_file(file, false); // lock for reading
    while (fgets(line, MAX_LINE_LEN, file))
    {
        if (sscanf(line, "%s", file_acct) == 1)
        {
            if (strcmp(file_acct, account_no) == 0)
            {
                found = true;
                break;
            }
        }
    }
    unlock_file(file);
    fclose(file);
    return found;
}

bool get_balance(const char *account_no, double *balance)
{
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file)
    {
        return false;
    }
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN];
    double file_balance;
    bool found = false;
    lock_file(file, false); // lock for reading
    while (fgets(line, MAX_LINE_LEN, file))
    {
        // Skip to the 6th field (balance)
        if (sscanf(line, "%s %*s %*s %*s %*s %lf", file_acct, &file_balance) == 2)
        {
            if (strcmp(file_acct, account_no) == 0)
            {
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

bool update_balance(const char *account_no, double new_balance)
{
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file)
    {
        return false;
    }
    FILE *temp = fopen("temp.txt", "w");
    if (!temp)
    {
        fclose(file);
        return false;
    }
    char line[MAX_LINE_LEN];
    char file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double file_balance;
    bool updated = false;
    lock_file(file, true);
    lock_file(temp, true);
    while (fgets(line, MAX_LINE_LEN, file))
    {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &file_balance) == 6)
        {
            if (strcmp(file_acct, account_no) == 0)
            {
                fprintf(temp, "%s %s %s %s %s %.2f\n", file_acct, file_pin, name, national_id, account_type, new_balance);
                updated = true;
            }
            else
            {
                fputs(line, temp);
            }
        }
        else
        {
            fputs(line, temp);
        }
    }
    unlock_file(file);
    unlock_file(temp);
    fclose(file);
    fclose(temp);
    if (remove(DB_FILENAME) != 0)
    {
        perror("Error removing file");
        return false;
    }
    if (rename("temp.txt", DB_FILENAME) != 0)
    {
        perror("Error renaming file");
        return false;
    }
    return updated;
}

bool add_account(const char *account_no, double initial_balance)
{
    FILE *file = fopen(DB_FILENAME, "a+");
    if (!file)
    {
        file = fopen(DB_FILENAME, "w");
        if (!file)
        {
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

void log_transaction(const char *account_no, const char *type, double amount, double balance)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE *file = fopen(filename, "a");
    if (!file)
        return;
    time_t now = time(NULL);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(file, "%s %.2f %.2f %s\n", type, amount, balance, timebuf);
    fclose(file);
}

// Helper to generate a random 4-digit PIN
static void generate_pin(char *pin_out)
{
    srand((unsigned int)time(NULL) ^ (unsigned int)rand());
    sprintf(pin_out, "%04d", 1000 + rand() % 9000);
}

// Helper to generate a unique account number (incremental)
static void generate_account_no(char *acct_out)
{
    FILE *file = fopen(DB_FILENAME, "r");
    int max_acct = 100000;
    char line[MAX_LINE_LEN], acct[MAX_ACCT_LEN];
    if (file)
    {
        while (fgets(line, sizeof(line), file))
            if (sscanf(line, "%s", acct) == 1)
            {
                int n = atoi(acct);
                if (n > max_acct)
                    max_acct = n;
            }
        fclose(file);
    }
    sprintf(acct_out, "%d", max_acct + 1);
}

// Extended Functions Implementation
bool open_account(const char *name, const char *national_id, const char *account_type, char *out_account_no, char *out_pin, double initial_deposit)
{
    if (!name || !national_id || !account_type || !out_account_no || !out_pin)
        return false;
    if (initial_deposit < 1000)
        return false;
    if (strcmp(account_type, "savings") != 0 && strcmp(account_type, "checking") != 0)
    {
        printf("Invalid account type. Must be 'savings' or 'checking'.\n");
        return false;
    }
    generate_account_no(out_account_no);
    generate_pin(out_pin);
    FILE *file = fopen(DB_FILENAME, "a+");
    if (!file)
    {
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

bool close_account(const char *account_no, const char *pin)
{
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file)
        return false;
    FILE *temp = fopen("temp.txt", "w");
    if (!temp)
    {
        fclose(file);
        return false;
    }
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool closed = false;
    lock_file(file, true);
    lock_file(temp, true);
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2)
        {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0)
            {
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

bool withdraw_extended(const char *account_no, const char *pin, double amount)
{
    if (amount < 500)
        return false;
    FILE *db = fopen(DB_FILENAME, "r+");
    if (!db)
        return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double balance = 0;
    bool found = false;
    while (fgets(line, sizeof(line), db))
    {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &balance) == 6)
        {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0)
            {
                found = true;
                break;
            }
        }
    }
    fclose(db);
    if (!found)
        return false;
    if (balance - amount < 1000)
        return false;
    if (!update_balance(account_no, balance - amount))
        return false;
    log_transaction(account_no, "WITHDRAW", amount, balance - amount);
    return true;
}

bool deposit_extended(const char *account_no, const char *pin, double amount)
{
    if (amount < 500)
        return false;
    FILE *db = fopen(DB_FILENAME, "r");
    if (!db)
        return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double balance = 0;
    bool found = false;
    while (fgets(line, sizeof(line), db))
    {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &balance) == 6)
        {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0)
            {
                found = true;
                break;
            }
        }
    }
    fclose(db);
    if (!found)
        return false;
    if (!update_balance(account_no, balance + amount))
        return false;
    log_transaction(account_no, "DEPOSIT", amount, balance + amount);
    return true;
}

bool balance(const char *account_no, double *out_balance)
{
    FILE *file = fopen(DB_FILENAME, "r");
    if (!file)
        return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN];
    double file_balance;
    bool found = false;
    while (fgets(line, sizeof(line), file))
    {
        // Skip to the 6th field (balance)
        if (sscanf(line, "%s %*s %*s %*s %*s %lf", file_acct, &file_balance) == 2)
        {
            if (strcmp(file_acct, account_no) == 0)
            {
                *out_balance = file_balance;
                found = true;
                break;
            }
        }
    }
    fclose(file);
    return found;
}

bool statement(const char *account_no, const char *pin, char transactions[5][MAX_LINE_LEN])
{
    FILE *db = fopen(DB_FILENAME, "r");
    if (!db)
        return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool found = false;
    while (fgets(line, sizeof(line), db))
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2 &&
            !strcmp(file_acct, account_no) && !strcmp(file_pin, pin))
        {
            found = true;
            break;
        }
    fclose(db);
    if (!found)
        return false;
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE *tf = fopen(filename, "r");
    if (!tf)
        return false;
    char all[5][MAX_LINE_LEN] = {{0}};
    int count = 0;
    while (fgets(line, sizeof(line), tf))
        count < 5 ? strncpy(all[count++], line, MAX_LINE_LEN) : (memmove(all, all + 1, 4 * MAX_LINE_LEN), strncpy(all[4], line, MAX_LINE_LEN));
    fclose(tf);
    for (int i = 0; i < 5; ++i)
        strncpy(transactions[i], all[i], MAX_LINE_LEN);
    return count > 0;
}

bool check_balance_extended(const char *account_no, const char *pin, double *balance)
{
    if (!account_no || !pin || !balance)
        return false;

    FILE *db = fopen(DB_FILENAME, "r");
    if (!db)
        return false;

    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8], name[64], national_id[32], account_type[16];
    double file_balance;
    bool found = false;

    while (fgets(line, sizeof(line), db))
    {
        if (sscanf(line, "%s %s %s %s %s %lf", file_acct, file_pin, name, national_id, account_type, &file_balance) == 6)
        {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0)
            {
                *balance = file_balance;
                found = true;
                break;
            }
        }
    }
    fclose(db);
    return found;
}

bool get_statement_extended(const char *account_no, const char *pin, char *statement, size_t statement_size)
{
    if (!account_no || !pin || !statement || statement_size == 0)
        return false;

    // First verify account and PIN
    FILE *db = fopen(DB_FILENAME, "r");
    if (!db)
        return false;

    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool found = false;

    while (fgets(line, sizeof(line), db))
    {
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2)
        {
            if (strcmp(file_acct, account_no) == 0 && strcmp(file_pin, pin) == 0)
            {
                found = true;
                break;
            }
        }
    }
    fclose(db);

    if (!found)
        return false;

    // Get transaction history
    char filename[64];
    snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE *tf = fopen(filename, "r");
    if (!tf)
        return false;

    // Initialize statement buffer
    memset(statement, 0, statement_size);
    char *current_pos = statement;
    size_t remaining_size = statement_size;

    // Read and format transactions
    while (fgets(line, sizeof(line), tf) && remaining_size > 0)
    {
        char type[32];
        double amount, balance;
        char timestamp[32];
        if (sscanf(line, "%s %lf %lf %s", type, &amount, &balance, timestamp) == 4)
        {
            int written = snprintf(current_pos, remaining_size,
                                   "%s: %.2f (Balance: %.2f) at %s\n",
                                   type, amount, balance, timestamp);

            if (written < 0 || (size_t)written >= remaining_size)
                break;

            current_pos += written;
            remaining_size -= written;
        }
    }
    fclose(tf);
    return true;
}

bool close_account_extended(const char *account_no, const char *pin, double *final_balance)
{
    if (!account_no || !pin || !final_balance)
        return false;

    // First get the current balance
    if (!check_balance_extended(account_no, pin, final_balance))
        return false;

    // Then close the account
    if (!close_account(account_no, pin))
        return false;

    return true;
}