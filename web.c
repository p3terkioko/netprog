#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h> // For getaddrinfo
#include <unistd.h> // For close

#define BUFFER_SIZE 4096

// Function to parse URL (you'll need to implement this robustly)
void parse_url(const char *url, char *hostname, char *path, int *port) {
    // Basic parsing: Assumes http://hostname/path
    // This is a very simplified example and needs significant improvement
    if (strncmp(url, "http://", 7) == 0) {
        url += 7; // Skip "http://"
    }

    char *path_start = strchr(url, '/');
    if (path_start) {
        strncpy(hostname, url, path_start - url);
        hostname[path_start - url] = '\0';
        strcpy(path, path_start);
    } else {
        strcpy(hostname, url);
        strcpy(path, "/"); // Default path
    }
    *port = 80; // Default HTTP port
}

int main() {
    char url[256];
    char hostname[256];
    char path[256];
    int port;

    printf("Enter the URL (e.g., http://www.example.com/): ");
    fgets(url, sizeof(url), stdin);
    url[strcspn(url, "\n")] = 0; // Remove newline character

    parse_url(url, hostname, path, &port);

    printf("Connecting to: %s (Port: %d), Path: %s\n", hostname, port, path);

    // 1. DNS Lookup
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream socket

    int status = getaddrinfo(hostname, "80", &hints, &res); // "80" for port
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    // 2. Create Socket
    int client_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client_socket == -1) {
        perror("Socket creation failed");
        freeaddrinfo(res);
        return 1;
    }

    // 3. Connect to Server
    if (connect(client_socket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Connection failed");
        close(client_socket);
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res); // Free the addrinfo structure once connected

    // 4. Construct HTTP GET Request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: MyCWebClient/1.0\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, hostname);

    printf("\nSending Request:\n%s\n", request);

    // 5. Send Request
    if (send(client_socket, request, strlen(request), 0) == -1) {
        perror("Send failed");
        close(client_socket);
        return 1;
    }

    // 6. Receive Response
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    printf("Received Response:\n");
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data
        printf("%s", buffer);
    }
    if (bytes_received == -1) {
        perror("Receive failed");
    }

    // 7. Close Socket
    close(client_socket);

    return 0;
}