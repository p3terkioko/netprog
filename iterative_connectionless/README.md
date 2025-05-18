# Iterative Connectionless Banking System

A simple banking system implemented using UDP (connectionless) protocol, featuring a client-server architecture with iterative processing.

## Overview

This system implements a basic banking application with the following features:

- Account registration
- Deposits
- Withdrawals
- Balance checking
- Transaction logging
- Account statements

## Components

### Server (`server.c`)

- Handles UDP connections on port 8080
- Processes banking operations
- Manages account data
- Provides responses to client requests

### Client (`client.c`)

- Interactive command-line interface
- Connects to the server using UDP
- Provides a user-friendly menu for banking operations
- Handles user input validation

### Bank Utilities (`bank_utils.c`)

- Core banking functionality
- Account management
- Transaction processing
- Data persistence

## Building the Project

The project uses a Makefile for building. To compile:

```bash
make
```

This will create two executables:

- `server`: The banking server
- `client`: The client application

To clean the build:

```bash
make clean
```

## Running the System

1. Start the server:

```bash
./server
```

2. In a separate terminal, start the client:

```bash
./client <server_ip> <server_port>
```

For example, to connect to the default local server:

```bash
./client 127.0.0.1 8080
```

## Features

### Account Management

- Register new accounts with initial deposit (minimum 1000)
- Account types: savings and checking
- PIN-based authentication

### Transactions

- Deposits (minimum 500)
- Withdrawals
- Balance checking
- Transaction history

### Security

- PIN verification
- Input validation
- Secure transaction logging

## Technical Details

- Protocol: UDP (User Datagram Protocol)
- Default server address: 127.0.0.1 (localhost)
- Default port: 8080
- Maximum message length: 256 bytes
- Maximum account number length: 16 characters

## Error Handling

The system handles various error conditions:

- Invalid account numbers
- Insufficient funds
- Invalid amounts
- Network errors
- Authentication failures
- Invalid command line arguments
- Connection failures

## Dependencies

- C compiler (gcc)
- Standard C libraries
- POSIX-compliant operating system

## Notes

- This is an iterative server implementation, meaning it processes one request at a time
- The system uses UDP for communication, which is connectionless and may not guarantee delivery
- All monetary values are stored as double-precision floating-point numbers
- Transaction logs are maintained for audit purposes
