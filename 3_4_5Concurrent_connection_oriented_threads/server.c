#include "common.h"

typedef struct {
    int sockfd;
    struct sockaddr_in address;
} client_t;

void* handle_client(void* arg) {
    client_t* client = (client_t*)arg;
    char buffer[MAX_MSG_LEN];
    int bytes_read;
    
    // Read client request
    while ((bytes_read = read(client->sockfd, buffer, sizeof(buffer)-1)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Process request
        char* response = process_request(buffer);
        
        // Send response
        write(client->sockfd, response, strlen(response));
        free(response);
        
        memset(buffer, 0, sizeof(buffer));
    }
    
    close(client->sockfd);
    free(client);
    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Banking server started on port %d\n", PORT);
    
    // Create transaction directory if not exists
    mkdir(TRANSACTION_LOG_DIR, 0777);
    
    // Accept connections
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        // Create client structure
        client_t* client = malloc(sizeof(client_t));
        client->sockfd = client_fd;
        client->address = client_addr;
        
        // Create thread for client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(client);
        } else {
            pthread_detach(thread_id);
        }
    }
    
    close(server_fd);
    return 0;
}