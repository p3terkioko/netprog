#include "common.h"

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