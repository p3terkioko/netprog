# README.md

# Concurrent TCP Banking Server

This project implements a concurrent TCP banking server and client using POSIX APIs. The server listens for incoming connections and handles multiple clients simultaneously by forking new processes for each request. The client interacts with the server to perform various banking operations.

## Project Structure

```
concurrent-server
├── src
│   ├── server.c        # Concurrent TCP banking server implementation
│   ├── client.c        # Client for the banking server
│   ├── bankv1.c        # Business logic for banking operations
│   ├── common.h        # Common definitions and function prototypes
│   └── utils.h         # Utility function prototypes
├── Makefile            # Build instructions
└── README.md           # Project documentation
```

## Features

- **Server**:
  - Listens on a configurable TCP port (default: 9000).
  - Accepts incoming client connections.
  - Forks a new process for each client request.
  - Supports commands:
    - `REGISTER <acct>`: Register a new account.
    - `DEPOSIT <acct> <amt>`: Deposit an amount into an account.
    - `WITHDRAW <acct> <amt>`: Withdraw an amount from an account.
    - `CHECK_BALANCE <acct>`: Check the balance of an account.
  - Uses file locking with `flock()` for serialized access to account data.
  - Logs transactions to individual account files (`<acct>.txt`).
  - Handles `SIGCHLD` to clean up zombie processes.

- **Client**:
  - Connects to the banking server using the server IP and port.
  - Presents a text menu for user interaction:
    - Register
    - Deposit
    - Withdraw
    - Check Balance
    - Exit
  - Reads user input, formats requests, and sends them to the server.
  - Displays server responses and loops until the user chooses to exit.

## Setup Instructions

1. Clone the repository:
   ```
   git clone <repository-url>
   cd concurrent-server
   ```

2. Build the project using the Makefile:
   ```
   make
   ```

3. Run the server:
   ```
   ./src/server
   ```

4. In a separate terminal, run the client:
   ```
   ./src/client <server_ip> <server_port>
   ```

## Usage Examples

- To register an account:
  ```
  REGISTER my_account
  ```

- To deposit money:
  ```
  DEPOSIT my_account 100
  ```

- To withdraw money:
  ```
  WITHDRAW my_account 50
  ```

- To check balance:
  ```
  CHECK_BALANCE my_account
  ```

## License

This project is licensed under the MIT License. See the LICENSE file for more details.