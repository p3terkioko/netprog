#ifndef COMMON_H
#define COMMON_H

// Common definitions and constants
#define MAX_BUFFER_SIZE 1024
#define PORT 8080

// Error codes
#define SUCCESS 0
#define ERROR -1
#define ERR_CONNECTION_FAILED 1
#define ERR_SEND_FAILED 2
#define ERR_RECEIVE_FAILED 3

// Message formats
#define MSG_HELLO "HELLO"
#define MSG_GOODBYE "GOODBYE"

// Function prototypes
void handle_error(const char* msg);
void trim_whitespace(char* str);
void format_message(char* buffer, const char* format, ...);

#endif // COMMON_H