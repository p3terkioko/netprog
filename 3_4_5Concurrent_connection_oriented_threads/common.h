#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#define PORT 8080
#define MAX_MSG_LEN 256
#define MAX_ACCT_LEN 16
#define MAX_AMT_LEN 16
#define DB_FILENAME "accounts.dat"
#define TRANSACTION_LOG_DIR "transactions"
#define MAX_LINE_LEN 256

// Operation codes
#define OP_REGISTER "REGISTER"
#define OP_DEPOSIT "DEPOSIT"
#define OP_WITHDRAW "WITHDRAW"
#define OP_CHECK "CHECK_BALANCE"
#define OP_STATEMENT "STATEMENT"
#define OP_CLOSE "CLOSE_ACCOUNT"

// Response codes
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_ACCT_EXISTS "ACCOUNT_EXISTS"
#define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
#define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
#define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
#define RESP_INVALID_PIN "INVALID_PIN"
#define RESP_INVALID_REQUEST "INVALID_REQUEST"

// Account structure
typedef struct {
    char account_no[MAX_ACCT_LEN];
    char pin[8];
    char name[64];
    char national_id[32];
    char account_type[16];
    double balance;
} Account;

// Function prototypes
char* create_message(const char* operation, const char* account_no, double amount);
bool parse_message(const char* message, char* operation, char* account_no, double* amount);
char* create_response(const char* status, double balance);
bool parse_response(const char* response, char* status, double* balance);
char* process_request(const char* request);

// File operations
bool account_exists(const char* account_no);
bool get_account(const char* account_no, Account* account);
bool update_account(const Account* account);
bool add_account(const Account* account);
bool delete_account(const char* account_no);
void log_transaction(const char* account_no, const char* type, double amount, double balance);
bool get_transactions(const char* account_no, char transactions[][MAX_LINE_LEN], int max_transactions);

// Utility functions
void generate_pin(char* pin);
bool validate_pin(const char* account_no, const char* pin);
bool is_valid_amount(double amount);
void trim_newline(char* str);

#endif