/* server.c - Fork-based concurrent server */
#include "common.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
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
// Business Logic Functions - will move to server.c in future versions
char* process_request(const char* request);
char* register_account(const char* account_no);
char* deposit(const char* account_no, double amount);
char* withdraw(const char* account_no, double amount);
char* check_balance(const char* account_no);
// Utility Functions
void trim(char* str);
bool is_valid_account_no(const char* account_no);
bool is_valid_amount(double amount);
static void generate_pin(char* pin_out);
// File I/O Functions - will move to server.c in future versions
bool account_exists(const char* account_no);
bool get_balance(const char* account_no, double* balance);
bool update_balance(const char* account_no, double new_balance);
bool add_account(const char* account_no, const char* pin, double initial_balance);
void handle_client(int client_sock);
// Extended function prototypes
bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit);
bool close_account(const char* account_no, const char* pin);
bool withdraw_extended(const char* account_no, const char* pin, double amount);
bool deposit_extended(const char* account_no, const char* pin, double amount);
bool balance(const char* account_no, double* out_balance);
bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN]);
void log_transaction(const char* account_no, const char* type, double amount, double balance);
// Add these prototypes
static void lock_file(FILE* file, bool exclusive);
static void unlock_file(FILE* file);
bool check_pin(const char* account_no, const char* pin);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        } else {
            close(client_sock);
        }
    }
    return 0;
}

void handle_client(int client_sock) {
    char buffer[MAX_MSG_LEN];
    char operation[32], account_no[MAX_ACCT_LEN], pin[8];
    double amount;
    int n = read(client_sock, buffer, sizeof(buffer)-1);
    if (n <= 0) return;
    buffer[n] = '\0';

    sscanf(buffer, "%s", operation);

    if (strcmp(operation, OP_REGISTER) == 0) {
        sscanf(buffer, "%*s %s", account_no);
        char* resp = register_account(account_no);
        write(client_sock, resp, strlen(resp));
        free(resp);
    } else if (strcmp(operation, OP_DEPOSIT) == 0) {
        sscanf(buffer, "%*s %s %s %lf", account_no, pin, &amount);
        if (!check_pin(account_no, pin)) {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
            return;
        }
        bool ok = deposit_extended(account_no, pin, amount);
        double bal = 0;
        if (ok && balance(account_no, &bal)) {
            char* resp = create_response(RESP_OK, bal);
            write(client_sock, resp, strlen(resp));
            free(resp);
        } else {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        }
    } else if (strcmp(operation, OP_WITHDRAW) == 0) {
        sscanf(buffer, "%*s %s %s %lf", account_no, pin, &amount);
        if (!check_pin(account_no, pin)) {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
            return;
        }
        bool ok = withdraw_extended(account_no, pin, amount);
        double bal = 0;
        if (ok && balance(account_no, &bal)) {
            char* resp = create_response(RESP_OK, bal);
            write(client_sock, resp, strlen(resp));
            free(resp);
        } else {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        }
    } else if (strcmp(operation, OP_CHECK) == 0) {
        sscanf(buffer, "%*s %s %s", account_no, pin);
        if (!check_pin(account_no, pin)) {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
            return;
        }
        double bal = 0;
        if (balance(account_no, &bal)) {
            char* resp = create_response(RESP_OK, bal);
            write(client_sock, resp, strlen(resp));
            free(resp);
        } else {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        }
    } else if (strcmp(operation, OP_STATEMENT) == 0) {
        sscanf(buffer, "%*s %s %s", account_no, pin);
        if (!check_pin(account_no, pin)) {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
            return;
        }
        char transactions[5][MAX_LINE_LEN] = {{0}};
        if (statement(account_no, pin, transactions)) {
            char resp[MAX_MSG_LEN*2] = "";
            for (int i = 0; i < 5; ++i)
                if (strlen(transactions[i]) > 0)
                    strcat(resp, transactions[i]);
            write(client_sock, resp, strlen(resp));
        } else {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        }
    } else if (strcmp(operation, OP_CLOSE) == 0) {
        sscanf(buffer, "%*s %s %s", account_no, pin);
        if (!check_pin(account_no, pin)) {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
            return;
        }
        bool ok = close_account(account_no, pin);
        if (ok) {
            char* resp = create_response(RESP_OK, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        } else {
            char* resp = create_response(RESP_ERROR, 0);
            write(client_sock, resp, strlen(resp));
            free(resp);
        }
    } else {
        char* resp = create_response(RESP_INVALID_REQUEST, 0.0);
        write(client_sock, resp, strlen(resp));
        free(resp);
    }
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}



// Business Logic Functions Implementation

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
    char pin[8];
    generate_pin(pin); // Make sure generate_pin is not static if you call it here
    // Store account_no and pin in DB (add_account should be updated to take pin)
    if (add_account(account_no, pin, 0)) {
        // Return PIN in response, e.g. "OK <PIN>"
        char* response = malloc(MAX_MSG_LEN);
        snprintf(response, MAX_MSG_LEN, "%s %s", RESP_OK, pin);
        return response;
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

bool add_account(const char* account_no, const char* pin, double initial_balance) {
    FILE* file = fopen(DB_FILENAME, "a+");
    if (!file) {
        file = fopen(DB_FILENAME, "w");
        if (!file) {
            perror("Error creating database file");
            return false;
        }
    }
    lock_file(file, true);
    // Add a default name, national_id, account_type if not provided
    fprintf(file, "%s %s %s %s %s %.2f\n", account_no, pin, "noname", "00000000", "savings", initial_balance);
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

bool check_pin(const char* account_no, const char* pin) {
    FILE* file = fopen(DB_FILENAME, "r");
    if (!file) return false;
    char line[MAX_LINE_LEN], file_acct[MAX_ACCT_LEN], file_pin[8];
    bool found = false;
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s", file_acct, file_pin) == 2 &&
            strcmp(file_acct, account_no) == 0 &&
            strcmp(file_pin, pin) == 0) {
            found = true;
            break;
        }
    }
    fclose(file);
    return found;
}