# Banking Client Documentation

## Overview

This client provides a menu-driven interface for users to interact with the banking server over TCP. It allows users to register, deposit, withdraw, check balance, view statements, and close accounts.

---

## How the Client Works

- The client displays a menu and prompts the user for input.
- Based on the user's choice, it collects the necessary information (account number, PIN, amount).
- It formats the request as a protocol message and sends it to the server via a TCP socket.
- The client waits for the server's response and displays it to the user.
- Each operation is performed in a new connection to the server.

**Main client loop:**

```c
while (1) {
    display_menu();
    scanf("%d", &choice); getchar();
    if (choice == 7) break;

    // Collect input and build request message...
    // Connect to server, send request, receive response...
}
```

---

## Supported Operations

| Menu Option      | Description                    | Example Input Sequence            |
| ---------------- | ------------------------------ | --------------------------------- |
| 1. Register      | Create a new account           | Enter account number              |
| 2. Deposit       | Deposit money into an account  | Enter account number, PIN, amount |
| 3. Withdraw      | Withdraw money from an account | Enter account number, PIN, amount |
| 4. Check Balance | Get the current balance        | Enter account number, PIN         |
| 5. Statement     | Get last 5 transactions        | Enter account number, PIN         |
| 6. Close Account | Close an account               | Enter account number, PIN         |
| 7. Exit          | Exit the client                |                                   |

---

## Example: Sending a Deposit Request

```c
printf("Enter account number: ");
fgets(account_no, sizeof(account_no), stdin); strtok(account_no, "\n");
printf("Enter PIN: ");
fgets(pin, sizeof(pin), stdin); strtok(pin, "\n");
printf("Enter amount: ");
scanf("%lf", &amount); getchar();
snprintf(buffer, sizeof(buffer), "%s %s %s %.2f", OP_DEPOSIT, account_no, pin, amount);

int sock = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in serv_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(PORT),
    .sin_addr.s_addr = inet_addr("127.0.0.1")
};
connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
write(sock, buffer, strlen(buffer));
int n = read(sock, response, sizeof(response)-1);
response[n] = '\0';
printf("Server: %s\n", response);
close(sock);
```

---

## Protocol Message Formats

- **Register:** `REGISTER <account_no>`
- **Deposit:** `DEPOSIT <account_no> <pin> <amount>`
- **Withdraw:** `WITHDRAW <account_no> <pin> <amount>`
- **Check Balance:** `CHECK_BALANCE <account_no> <pin>`
- **Statement:** `STATEMENT <account_no> <pin>`
- **Close Account:** `CLOSE_ACCOUNT <account_no> <pin>`

---

## Notes

- All communication is over TCP to `127.0.0.1:8080`.
- Each operation opens a new connection to the server.
- PIN is required for all sensitive operations.
- Responses from the server are displayed as plain text.

---

For more details, see [`client.c`](client.c).
