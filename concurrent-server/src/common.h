#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define SERVER_PORT 9000
#define BUFFER_SIZE 1024
#define DATA_FILE "data.txt"

void register_account(const char *account);
void deposit(const char *account, double amount);
void withdraw(const char *account, double amount);
double check_balance(const char *account);
void log_transaction(const char *account, const char *transaction);

#endif // COMMON_H