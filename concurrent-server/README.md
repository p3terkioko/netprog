# Concurrent Connection-Oriented Server

This project implements a connection-oriented concurrent server using processes for Unix systems. The server listens for incoming client connections and forks a new process for each connection to handle client requests.

## Project Structure

```
concurrent-server
├── src
│   ├── server.c        # Implements the server functionality
│   ├── client.c        # Implements the client application
│   ├── common.h        # Contains common definitions and prototypes
│   └── utils.c         # Contains utility functions
├── Makefile             # Build script for compiling the project
└── README.md            # Project documentation
```

## Compilation

To compile the project, navigate to the project directory and run:

```
make
```

This will generate the server and client executables.

## Running the Server

To start the server, run:

```
./server
```

The server will listen for incoming connections on a predefined port.

## Running the Client

To connect to the server, run:

```
./client
```

The client will send requests to the server and display the responses.

## Configuration

Make sure to configure the server port and any other necessary parameters in the source files before running the server and client.

## Dependencies

This project requires a Unix-like environment with support for process management and sockets. Ensure that you have the necessary development tools installed.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.