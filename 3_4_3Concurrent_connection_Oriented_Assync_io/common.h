#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

// Constants
#define MAX_LINE_LEN 1024
#define MAX_MSG_LEN 1536 // Increased for statement data
#define MAX_ACCT_LEN 10
#define MAX_AMT_LEN 10

// Message operation codes
#define OP_REGISTER "REGISTER"
#define OP_DEPOSIT "DEPOSIT"
#define OP_WITHDRAW "WITHDRAW"
#define OP_CHECK "CHECK"
#define OP_STATEMENT "STATEMENT"
#define OP_CLOSE_ACCOUNT "CLOSE_ACCOUNT"
#define OP_OPEN_ACCOUNT "OPEN_ACCOUNT"

// Response codes
#define RESP_OK "OK"
#define RESP_ERROR "ERROR"
#define RESP_ACCT_EXISTS "ACCOUNT_EXISTS"
#define RESP_ACCT_NOT_FOUND "ACCOUNT_NOT_FOUND"
#define RESP_INSUFFICIENT_FUNDS "INSUFFICIENT_FUNDS"
#define RESP_INVALID_AMOUNT "INVALID_AMOUNT"
#define RESP_INVALID_REQUEST "INVALID_REQUEST"

// Shared definitions
#define SERVER_PORT 12345
#define MAX_EVENTS 10 // For epoll

#endif // COMMON_H
