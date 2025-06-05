# Design Document: Concurrent Banking Application

## 1. Introduction

This document outlines the design of the refactored banking application, which transforms a monolithic C program (`bankv1.c`) into a concurrent, connection-oriented client-server system. The server utilizes asynchronous I/O with `epoll(7)` for efficient handling of multiple clients on Linux.

## 2. Architecture Overview

The system is composed of two main components:

*   **`server.c` (Server):** Handles all business logic, data persistence, and manages client connections. It's designed to be highly concurrent using an event-driven model.
*   **`client.c` (Client):** Provides the user interface, constructs requests based on user input, sends them to the server, and displays responses.

Communication between client and server is TCP-based, using a defined message protocol.

## 3. Server Design (`server.c`)

### 3.1. Concurrency Model: Event-Driven with Asynchronous I/O

The server employs a single-threaded event loop model built around the Linux `epoll` API. This allows the server to manage many concurrent client connections efficiently without resorting to a thread-per-client model, minimizing overhead and context switching.

*   **Non-Blocking Sockets:** All sockets (the main listening socket and accepted client sockets) are set to non-blocking mode using `fcntl()`. This ensures that operations like `accept()`, `read()`, and `send()` do not block the server's main thread.
*   **`epoll` Usage:**
    *   An `epoll` instance is created using `epoll_create1()`.
    *   The listening socket is added to the `epoll` set, monitored for input events (`EPOLLIN`) using edge-triggered notification (`EPOLLET`).
    *   When `epoll_wait()` returns an event for the listening socket, new client connections are `accept()`ed. Due to `EPOLLET`, `accept()` is called in a loop until it returns `EAGAIN` or `EWOULDBLOCK`.
    *   New client sockets are also set to non-blocking and added to the `epoll` set, monitored for `EPOLLIN | EPOLLET`.
    *   **Client Data (EPOLLIN):** When data arrives from a client, `read()` is called in a loop (due to `EPOLLET`) to read all available data. The complete message is then parsed and processed by `handle_client_operation()`. The response is sent back to the client.
    *   **Client Data (EPOLLOUT):** Basic support for `EPOLLOUT` is included. If `send()` would block, `EPOLLOUT` is registered for the socket. When the socket becomes writable, the server is notified. (Note: The current implementation's EPOLLOUT handling is basic and primarily serves to unregister itself to prevent busy loops if a send initially blocks; full buffering of large pending writes per client is a potential future enhancement).
    *   **Error Handling:** `EPOLLERR` and `EPOLLHUP` events, as well as disconnections detected by `read()` returning 0, lead to the proper cleanup of client sockets (closing and removing from `epoll`).

### 3.2. Business Logic and Message Handling

*   Business logic functions (e.g., `open_account`, `deposit_extended`, `check_balance`, `statement`) are encapsulated within `server.c`.
*   `handle_client_operation()` is the central function that takes a raw message string from a client.
*   `parse_message()` (using `strtok`) decodes the client's request into an operation code and arguments.
*   Based on the operation, the corresponding business logic function is invoked.
*   `create_response()` formats the result (or error) into a string to be sent back to the client.

### 3.3. File Persistence and Locking

*   Account master data is stored in `data.txt` (relative to where the server is run).
*   Transaction history for each account is stored in `[account_no].txt` (relative to where the server is run).
*   **File Locking:** The `flock(2)` system call is used to manage access to these files.
    *   Shared locks (`LOCK_SH`) are used for read-only operations (e.g., checking balance, generating a statement).
    *   Exclusive locks (`LOCK_EX`) are used for write operations (e.g., deposits, withdrawals, new account registration, logging transactions).
    *   For critical updates to `data.txt` (e.g., `update_balance`, `close_account`), a common pattern of writing to a temporary file and then atomically renaming it is used. The original `data.txt` is exclusively locked during this process to ensure consistency.
    *   Since the server's event loop is single-threaded, `flock` primarily serves to prevent concurrent access issues if multiple instances of the server were run against the same data files or if other external processes attempt to modify the files. Within the single server process, request processing is serialized, preventing race conditions in the business logic itself.

## 4. Client Design (`client.c`)

*   The client implements the user interface functions (`display_menu`, `handle_user_input`, etc.).
*   It establishes a TCP connection to the server.
*   User input is translated into request strings using `create_message()`. This function formats messages like "OPERATION arg1 arg2...".
*   Requests are sent to the server, and responses are received.
*   `parse_response_status()` extracts the status code from the server's response. Based on the status and the original operation, `handle_user_input()` further parses arguments from the response (e.g., account number/PIN for `OP_OPEN_ACCOUNT`, balance for `OP_CHECK`, or the transaction list for `OP_STATEMENT`).
*   The client uses blocking I/O for simplicity. It includes basic error handling for server disconnections.

## 5. Communication Protocol

*   The protocol is text-based, with messages typically consisting of an operation code followed by space-separated arguments.
*   Server responses start with a status code (e.g., `RESP_OK`, `RESP_ERROR`, `RESP_ACCT_NOT_FOUND`) followed by optional arguments.
*   `MAX_MSG_LEN` (defined in `common.h`) dictates the maximum size of messages and responses, set to 1536 bytes to accommodate potentially long statements.
*   Key operation codes (`OP_*`) and response codes (`RESP_*`) are defined in `common.h`.

## 6. Significant Modifications from `bankv1.c`

*   **Architecture:** Monolithic structure split into distinct client and server applications.
*   **Concurrency:** Server upgraded from sequential processing to concurrent handling of multiple clients using `epoll` and non-blocking I/O.
*   **Communication:** Local function calls replaced with network communication over TCP sockets using a defined message protocol.
*   **Message Parsing:** `parse_message` and `create_response` (server-side) and `create_message` and `parse_response_status` (client-side) were implemented/adapted for network protocol needs.
*   **Error Handling:** Network-related error handling added to both client and server. Server's `epoll` loop includes handling for client disconnections and socket errors.
