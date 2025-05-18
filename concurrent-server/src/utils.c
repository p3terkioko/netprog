#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Function to handle errors and print a message
void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Function to trim whitespace from a string
void trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Null terminate after the last non-space character
    *(end + 1) = '\0';
}

// Function to format a string for output
void format_string(char *dest, const char *src, size_t size) {
    snprintf(dest, size, "%s", src);
}