# Banking Server Documentation

## Overview

This server implements a concurrent TCP banking service using forked processes. It supports multiple clients simultaneously and provides basic banking operations such as registration, deposit, withdrawal, balance inquiry, statement, and account closure.

---

## How the Server Works

- The server listens for TCP connections on a specified port (default: `8080`).
- For each incoming client, the server forks a new process to handle the session.
- Each client request is parsed and dispatched to the appropriate handler function.
- Account data is stored in a text file (`data.txt`), and each account has a transaction log file.
- File locking is used to ensure safe concurrent access.

**Main server loop:**

```c
int server_sock, client_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t client_len = sizeof(client_addr);

server_sock = socket(AF_INET, SOCK_STREAM, 0);
bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
listen(server_sock, 5);

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
```

---

## Supported Operations

| Operation     | Description                    | Protocol Example                       |
| ------------- | ------------------------------ | -------------------------------------- |
| Register      | Create a new account           | `REGISTER <account_no>`                |
| Deposit       | Deposit money into an account  | `DEPOSIT <account_no> <pin> <amount>`  |
| Withdraw      | Withdraw money from an account | `WITHDRAW <account_no> <pin> <amount>` |
| Check Balance | Get the current balance        | `CHECK_BALANCE <account_no> <pin>`     |
| Statement     | Get last 5 transactions        | `STATEMENT <account_no> <pin>`         |
| Close Account | Close an account               | `CLOSE_ACCOUNT <account_no> <pin>`     |

---

## Example: Handling a Deposit

```c
else if (strcmp(operation, OP_DEPOSIT) == 0) {
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
}
```

---

## File Locking Example

To ensure safe concurrent access to the account database:

```c
static void lock_file(FILE* file, bool exclusive) {
    if (exclusive) {
        flock(fileno(file), LOCK_EX);
    } else {
        flock(fileno(file), LOCK_SH);
    }
}
```

---

## Transaction Logging

Each account has a transaction log file named `<account_no>.txt`:

```c
void log_transaction(const char* account_no, const char* type, double amount, double balance) {
    char filename[64]; snprintf(filename, sizeof(filename), "%s.txt", account_no);
    FILE* file = fopen(filename, "a"); if (!file) return;
    time_t now = time(NULL); char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(file, "%s %.2f %.2f %s\n", type, amount, balance, timebuf);
    fclose(file);
}
```

---

## Notes

- Minimum deposit/withdrawal is 500.
- Minimum balance after withdrawal must be 1000.
- PIN authentication is required for sensitive operations.
- Transaction logs are kept per account.

---

For more details, see [`server.c`](server.c).
