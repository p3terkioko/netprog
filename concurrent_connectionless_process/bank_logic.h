// bank_logic.h
#ifndef BANK_LOGIC_H
#define BANK_LOGIC_H

#include <stdbool.h>

#define DB_FILENAME "data.txt"
#define MAX_LINE_LEN 256
#define MAX_MSG_LEN 256
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16

// Operation codes for client-server communication
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

// Message formatting functions
char* create_message(const char* operation, const char* account_no, double amount);
bool parse_message(const char* message, char* operation, char* account_no, double* amount);
char* create_response(const char* status, double balance);
bool parse_response(const char* response, char* status, double* balance);

// Business logic API
char* process_request_extended(const char* request);
char* register_account(const char* account_no);
char* deposit(const char* account_no, double amount);
char* withdraw(const char* account_no, double amount);
char* check_balance(const char* account_no);

// Extended operations
bool open_account(const char* name, const char* national_id, const char* account_type, char* out_account_no, char* out_pin, double initial_deposit);
bool close_account(const char* account_no, const char* pin);
bool withdraw_extended(const char* account_no, const char* pin, double amount);
bool deposit_extended(const char* account_no, const char* pin, double amount);
bool balance(const char* account_no, double* out_balance);
bool statement(const char* account_no, const char* pin, char transactions[5][MAX_LINE_LEN]);
void log_transaction(const char* account_no, const char* type, double amount, double balance);

// File I/O
bool account_exists(const char* account_no);
bool get_balance(const char* account_no, double* balance);
bool update_balance(const char* account_no, double new_balance);
bool add_account(const char* account_no, double initial_balance);

// Utility
void trim(char* str);
bool is_valid_account_no(const char* account_no);
bool is_valid_amount(double amount);

#endif
