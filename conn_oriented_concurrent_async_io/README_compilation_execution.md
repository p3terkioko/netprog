# Banking Application - Client-Server Edition

## Compilation

To compile the server and client applications, ensure you have `gcc` and `make` installed on your Linux system.

1.  Open a terminal.
2.  Navigate to this directory (`conn_oriented_concurrent_async_io`).
3.  Clean any previous builds (optional, but good practice):
    ```bash
    make clean
    ```
4.  Compile the server and client:
    ```bash
    make
    ```
    This will produce two executables in the current directory: `server` and `client`.

## Running the Application

### 1. Start the Server

*   Ensure you are in the `conn_oriented_concurrent_async_io` directory in your terminal.
*   Run the server application:
    ```bash
    ./server
    ```
*   By default, the server will listen on port 12345 (as defined by `SERVER_PORT` in `common.h`).
*   You can optionally specify a different port number as a command-line argument:
    ```bash
    ./server <port_number>
    ```
    For example:
    ```bash
    ./server 8080
    ```
*   The server will print a message indicating it's listening, e.g., "Server started. Waiting for connections on port 12345 using epoll...".
*   Keep this terminal window open. The server needs to be running for clients to connect.

### 2. Connect Client(s)

*   Open one or more new terminal windows for each client you want to run.
*   In each client terminal, ensure you are in the `conn_oriented_concurrent_async_io` directory.
*   Run the client application:
    ```bash
    ./client
    ```
*   By default, the client will attempt to connect to `127.0.0.1` (localhost) on port 12345.
*   If the server is running on a different IP address or port, specify them as command-line arguments:
    ```bash
    ./client <server_ip_address> <server_port_number>
    ```
    For example, if the server is on the same machine but on port 8080:
    ```bash
    ./client 127.0.0.1 8080
    ```
    If the server is on a different machine (e.g., IP 192.168.1.100) on port 12345:
    ```bash
    ./client 192.168.1.100 12345
    ```
*   Once connected, the client will display the banking menu. You can interact with the menu to perform banking operations.
*   You can run multiple client instances simultaneously, each in its own terminal, to test concurrent operations against the server.

### Example Workflow

1.  **Terminal 1 (Server):**
    ```bash
    cd /path/to/repo/conn_oriented_concurrent_async_io
    make
    ./server
    ```
    *(Server is now running and listening)*

2.  **Terminal 2 (Client 1):**
    ```bash
    cd /path/to/repo/conn_oriented_concurrent_async_io
    ./client
    ```
    *(Client 1 connects, banking menu appears. Perform operations.)*

3.  **Terminal 3 (Client 2):**
    ```bash
    cd /path/to/repo/conn_oriented_concurrent_async_io
    ./client
    ```
    *(Client 2 connects, banking menu appears. Perform operations concurrently with Client 1.)*
