#ifndef BANK_UTILS_H
#define BANK_UTILS_H

#include <stdbool.h>

// Constants
#define DB_FILENAME "data.txt"
#define MAX_LINE_LEN 256
#define MAX_MSG_LEN 256
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

// Message operation codes
#define OP_REGISTER "REGISTER"
#define OP_DEPOSIT "DEPOSIT"
#define OP_WITHDRAW "WITHDRAW"
#define OP_CHECK "CHECK_BALANCE"

// Response codes
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_ACCT_EXISTS "ACCOUNT_EXISTS"
#define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
#define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
#define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
#define RESP_INVALID_REQUEST "INVALID_REQUEST"

// Function declarations
bool open_account(const char *name, const char *national_id, const char *account_type,
                  char *account_no, char *pin, double initial_deposit);
bool deposit_extended(const char *account_no, const char *pin, double amount);
bool withdraw_extended(const char *account_no, const char *pin, double amount);
bool check_balance_extended(const char *account_no, const char *pin, double *balance);
bool get_statement_extended(const char *account_no, const char *pin, char *statement, size_t statement_size);
bool close_account_extended(const char *account_no, const char *pin, double *final_balance);
bool is_valid_account_no(const char *account_no);
bool is_valid_amount(double amount);
void trim(char *str);

// File I/O Functions
bool account_exists(const char *account_no);
bool get_balance(const char *account_no, double *balance);
bool update_balance(const char *account_no, double new_balance);
bool add_account(const char *account_no, double initial_balance);
void log_transaction(const char *account_no, const char *type, double amount, double balance);

// Extended Functions
bool balance(const char *account_no, double *out_balance);
bool statement(const char *account_no, const char *pin, char transactions[5][MAX_LINE_LEN]);

#endif // BANK_UTILS_H