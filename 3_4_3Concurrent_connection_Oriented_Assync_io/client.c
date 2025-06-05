#define _GNU_SOURCE // Must be first
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h> // For isspace, isdigit
#include <errno.h> // For perror

// Global socket file descriptor
int client_socket_fd = -1;

// UI Functions
void display_menu();
void clear_input_buffer();
void clear_screen();
void wait_for_enter();

// Utility Functions
void trim(char* str);
bool is_valid_account_no(const char* account_no);
bool is_valid_amount_str(const char* amount_str); // Changed to validate string before atof

// Message Functions (Adapted)
char* create_message(const char* operation, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5);
// Renamed and simplified parse_response to only extract status. Caller handles further parsing.
bool parse_response_status(const char* response, char* status);

// Network Function
int connect_to_server(const char* ip_address, int port);

// Main application logic
void handle_user_input(int sock_fd);


// UI Functions Implementation
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    // Standard ANSI escape sequence to clear screen and move cursor to home
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void display_menu() {
    clear_screen();
    printf("\nBANKING MENU (Client)\n");
    printf("=======================\n");
    printf("1. Open New Account\n");        // Was Register New Account, maps to OP_OPEN_ACCOUNT
    printf("2. Deposit Funds\n");           // Maps to OP_DEPOSIT
    printf("3. Withdraw Funds\n");          // Maps to OP_WITHDRAW
    printf("4. Check Balance\n");          // Maps to OP_CHECK
    printf("5. View Account Statement\n"); // Maps to OP_STATEMENT
    printf("6. Close Account\n");          // Maps to OP_CLOSE_ACCOUNT
    printf("0. Exit\n");
    printf("\nEnter choice: ");
}

void wait_for_enter() {
    printf("\nPress ENTER to continue...");
    clear_input_buffer(); // Clear current line
    getchar(); // Wait for Enter key
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Utility Functions Implementation
void trim(char* str) {
    if (!str) return;
    char* start = str;
    while (isspace((unsigned char)*start)) start++;

    if (*start == 0) { // all spaces?
        *str = 0;
        return;
    }

    char* end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;

    if (start > str) memmove(str, start, strlen(start) + 1);
}

bool is_valid_account_no(const char* account_no) {
    if (!account_no || strlen(account_no) == 0) return false;
    for (int i = 0; account_no[i] != '\0'; i++) {
        if (!isdigit(account_no[i])) return false;
    }
    // Could add length checks if MAX_ACCT_LEN is strict from server perspective
    return true;
}

bool is_valid_amount_str(const char* amount_str) {
    if (!amount_str || !*amount_str) return false;
    int dot_count = 0;
    for (const char* p = amount_str; *p; ++p) {
        if (*p == '.') {
            dot_count++;
            if (dot_count > 1) return false; // Max one decimal point
        } else if (!isdigit(*p)) {
            return false; // Only digits and at most one dot
        }
    }
    double amount = atof(amount_str);
    return amount > 0; // Basic check, server might have more rules (e.g. minimums)
}


// Message Functions Implementation (Adapted)
// Modified create_message in client.c for flexibility
char* create_message(const char* operation, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5) {
    static char message[MAX_MSG_LEN]; // Static buffer, be careful with multi-threading if ever used
    memset(message, 0, MAX_MSG_LEN);

    strncat(message, operation, MAX_MSG_LEN - strlen(message) -1 );
    if (arg1) { strncat(message, " ", MAX_MSG_LEN - strlen(message) - 1); strncat(message, arg1, MAX_MSG_LEN - strlen(message) -1); }
    if (arg2) { strncat(message, " ", MAX_MSG_LEN - strlen(message) - 1); strncat(message, arg2, MAX_MSG_LEN - strlen(message) -1); }
    if (arg3) { strncat(message, " ", MAX_MSG_LEN - strlen(message) - 1); strncat(message, arg3, MAX_MSG_LEN - strlen(message) -1); }
    if (arg4) { strncat(message, " ", MAX_MSG_LEN - strlen(message) - 1); strncat(message, arg4, MAX_MSG_LEN - strlen(message) -1); }
    if (arg5) { strncat(message, " ", MAX_MSG_LEN - strlen(message) - 1); strncat(message, arg5, MAX_MSG_LEN - strlen(message) -1); }
    return message;
}

// Renamed and simplified parse_response to only extract status. Caller handles further parsing.
bool parse_response_status(const char* response, char* status) {
    if (!response || !status) return false;
    status[0] = '\0'; // Initialize status buffer
    // sscanf will read the first whitespace-separated token into status.
    int count = sscanf(response, "%s", status);
    return count == 1; // Successfully parsed one item (the status)
}


// Network Function Implementation
int connect_to_server(const char* ip_address, int port) {
    int sock_fd;
    struct sockaddr_in server_addr;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(sock_fd);
        return -1;
    }
    printf("Connected to server %s:%d\n", ip_address, port);
    return sock_fd;
}

// Main application logic
void handle_user_input(int sock_fd) {
    int choice;
    char account_no[MAX_ACCT_LEN + 1], pin[10], amount_str[MAX_AMT_LEN + 1]; // +1 for null terminator
    char name[64], national_id[32], account_type[16];

    char request_msg_buf[MAX_MSG_LEN];
    char recv_buffer[MAX_MSG_LEN]; // Uses new MAX_MSG_LEN from common.h
    char resp_status[MAX_LINE_LEN]; // For the status string from response

    // Buffers for additional arguments from response, sized for general cases or specific large data
    char resp_arg1_generic[MAX_LINE_LEN];
    char resp_arg2_generic[MAX_LINE_LEN];
    char statement_data_buffer[MAX_MSG_LEN]; // Specific large buffer for statement data

    while (1) {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            wait_for_enter();
            continue;
        }
        clear_input_buffer(); // Consume the newline after scanf

        if (choice == 0) {
            printf("Exiting client. Goodbye!\n");
            break;
        }

        // Clear buffers for safety before reuse
        memset(request_msg_buf, 0, sizeof(request_msg_buf));
        memset(recv_buffer, 0, sizeof(recv_buffer));
        memset(resp_status, 0, sizeof(resp_status));
        memset(resp_arg1_generic, 0, sizeof(resp_arg1_generic));
        memset(resp_arg2_generic, 0, sizeof(resp_arg2_generic));
        memset(statement_data_buffer, 0, sizeof(statement_data_buffer));

        int current_operation_choice = choice; // To identify operation in response handling

        switch (choice) {
            case 1: { // Open New Account (OP_OPEN_ACCOUNT)
                clear_screen();
                printf("=== OPEN NEW ACCOUNT ===\n");
                printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

                printf("Enter Full Name: ");
                if (!fgets(name, sizeof(name), stdin)) break;
                trim(name); if (strcmp(name, "b") == 0) break;

                printf("Enter National ID: ");
                if (!fgets(national_id, sizeof(national_id), stdin)) break;
                trim(national_id); if (strcmp(national_id, "b") == 0) break;

                printf("Enter Account Type (savings/checking): ");
                if (!fgets(account_type, sizeof(account_type), stdin)) break;
                trim(account_type); if (strcmp(account_type, "b") == 0) break;
                if (strcmp(account_type, "savings") != 0 && strcmp(account_type, "checking") != 0) {
                    printf("Invalid account type. Must be 'savings' or 'checking'.\n");
                    wait_for_enter(); break;
                }

                printf("Enter Initial Deposit Amount (min 1000.00): ");
                if (!fgets(amount_str, sizeof(amount_str), stdin)) break;
                trim(amount_str); if (strcmp(amount_str, "b") == 0) break;
                if (!is_valid_amount_str(amount_str) || atof(amount_str) < 1000.0) {
                     printf("Invalid or insufficient deposit amount (min 1000.00).\n");
                     wait_for_enter(); break;
                }

                // Message: OP_OPEN_ACCOUNT <name> <national_id> <account_type> <initial_deposit>
                char* msg = create_message(OP_OPEN_ACCOUNT, name, national_id, account_type, amount_str, NULL);
                strncpy(request_msg_buf, msg, MAX_MSG_LEN -1);

                if (send(sock_fd, request_msg_buf, strlen(request_msg_buf), 0) < 0) {
                    perror("Send failed"); client_socket_fd = -1; break; // Indicate connection lost
                }
                int bytes_received = recv(sock_fd, recv_buffer, MAX_MSG_LEN - 1, 0);
                if (bytes_received <= 0) {
                    perror(bytes_received == 0 ? "Server disconnected" : "Recv failed"); client_socket_fd = -1; break;
                }
                recv_buffer[bytes_received] = '\0';

                if (parse_response_status(recv_buffer, resp_status)) {
                    if (strcmp(resp_status, RESP_OK) == 0) {
                        // Expecting two arguments: new_account_no and new_pin
                        if (sscanf(recv_buffer, "%*s %s %s", resp_arg1_generic, resp_arg2_generic) == 2) {
                             printf("Account successfully created!\nAccount Number: %s\nPIN: %s\n", resp_arg1_generic, resp_arg2_generic);
                        } else {
                             printf("Server Error: OK response for OPEN_ACCOUNT but missing account_no/pin details from server.\n");
                        }
                    } else {
                        // Try to get an error message if server sent one after status
                        char error_details[MAX_LINE_LEN] = "";
                        // Read the rest of the line after the status token
                        sscanf(recv_buffer, "%*s %255[^\n]", error_details);
                        printf("Server Error: %s %s\n", resp_status, error_details);
                    }
                } else {
                    printf("Failed to parse server response status: %s\n", recv_buffer);
                }
                wait_for_enter();
                break;
            }
            case 2: // Deposit (OP_DEPOSIT)
            case 3: // Withdraw (OP_WITHDRAW)
            case 4: // Check Balance (OP_CHECK)
            case 5: // Statement (OP_STATEMENT)
            case 6: { // Close Account (OP_CLOSE_ACCOUNT)
                clear_screen();
                const char* op_name_str = ""; // User-friendly operation name
                const char* op_code_str = ""; // Actual operation code string for the message
                bool needs_amount_input = false;
                // current_operation_choice is already set

                if (choice == 2) { op_name_str = "DEPOSIT"; op_code_str = OP_DEPOSIT; needs_amount_input = true; }
                else if (choice == 3) { op_name_str = "WITHDRAW"; op_code_str = OP_WITHDRAW; needs_amount_input = true; }
                else if (choice == 4) { op_name_str = "CHECK BALANCE"; op_code_str = OP_CHECK; }
                else if (choice == 5) { op_name_str = "ACCOUNT STATEMENT"; op_code_str = OP_STATEMENT; }
                else if (choice == 6) { op_name_str = "CLOSE ACCOUNT"; op_code_str = OP_CLOSE_ACCOUNT; }

                printf("=== %s ===\n", op_name_str);
                printf("Type 'b' and press ENTER at any prompt to return to the main menu.\n");

                printf("Enter Account Number: ");
                if (!fgets(account_no, sizeof(account_no), stdin)) break;
                trim(account_no); if (strcmp(account_no, "b") == 0) break;
                if (!is_valid_account_no(account_no)) {
                    printf("Invalid account number format.\n"); wait_for_enter(); break;
                }

                printf("Enter PIN: ");
                if (!fgets(pin, sizeof(pin), stdin)) break;
                trim(pin); if (strcmp(pin, "b") == 0) break;
                // TODO: Basic PIN validation (e.g. length 4, digits only) can be added client-side

                if (needs_amount_input) {
                    printf("Enter Amount: ");
                    if (!fgets(amount_str, sizeof(amount_str), stdin)) break;
                    trim(amount_str); if (strcmp(amount_str, "b") == 0) break;
                    if (!is_valid_amount_str(amount_str)) { // Validates format and >0
                        printf("Invalid amount format or non-positive value.\n"); wait_for_enter(); break;
                    }
                }

                // Message: <OP_CODE> <account_no> <pin> [<amount>]
                char* msg = needs_amount_input ?
                    create_message(op_code_str, account_no, pin, amount_str, NULL, NULL) :
                    create_message(op_code_str, account_no, pin, NULL, NULL, NULL);
                strncpy(request_msg_buf, msg, MAX_MSG_LEN-1);

                if (send(sock_fd, request_msg_buf, strlen(request_msg_buf), 0) < 0) {
                    perror("Send failed"); client_socket_fd = -1; break;
                }
                int bytes_received = recv(sock_fd, recv_buffer, MAX_MSG_LEN - 1, 0);
                 if (bytes_received <= 0) {
                    perror(bytes_received == 0 ? "Server disconnected" : "Recv failed"); client_socket_fd = -1; break;
                }
                recv_buffer[bytes_received] = '\0';

                if (parse_response_status(recv_buffer, resp_status)) {
                    if (strcmp(resp_status, RESP_OK) == 0) {
                        // Further parse arguments based on operation
                        if (current_operation_choice == 4) { // Check Balance (OP_CHECK)
                            // Expects balance in arg1_generic
                            if (sscanf(recv_buffer, "%*s %s", resp_arg1_generic) == 1) {
                                printf("Current Balance: %s\n", resp_arg1_generic);
                            } else {
                                printf("Server Error: OK response for CHECK_BALANCE but missing balance data.\n");
                            }
                        } else if (current_operation_choice == 5) { // Statement (OP_STATEMENT)
                            // The rest of the buffer after "STATUS " is the statement data
                            const char* statement_payload_ptr = recv_buffer + strlen(resp_status);
                            if (*statement_payload_ptr == ' ') statement_payload_ptr++; // Skip the leading space if present

                            if (strlen(statement_payload_ptr) > 0) {
                                // Copy payload to dedicated buffer for strtok
                                strncpy(statement_data_buffer, statement_payload_ptr, MAX_MSG_LEN -1);
                                statement_data_buffer[MAX_MSG_LEN-1] = '\0'; // Ensure null termination

                                printf("--- Account Statement ---\n");
                                char* transactions_copy = strdup(statement_data_buffer);
                                if (transactions_copy) {
                                    char* token = strtok(transactions_copy, ";");
                                    if (token == NULL && strlen(statement_data_buffer) > 0) { // No delimiter, but data exists
                                        printf("%s\n", statement_data_buffer); // Print as single line if no ';'
                                    } else {
                                        while (token != NULL) {
                                            printf("%s\n", token);
                                            token = strtok(NULL, ";");
                                        }
                                    }
                                    free(transactions_copy);
                                } else {
                                    perror("strdup failed for statement processing");
                                    printf("Raw statement data: %s\n", statement_data_buffer); // Fallback
                                }
                                printf("--- End of Statement ---\n");
                            } else {
                                 printf("No transactions in statement or statement data empty.\n");
                            }
                        } else if (current_operation_choice == 2 || current_operation_choice == 3) { // DEPOSIT or WITHDRAW
                             // Expects new balance in arg1_generic
                            if (sscanf(recv_buffer, "%*s %s", resp_arg1_generic) == 1) {
                                printf("%s successful. New Balance: %s\n", op_name_str, resp_arg1_generic);
                            } else {
                                printf("%s successful. New Balance data missing from response.\n", op_name_str);
                            }
                        }
                        else { // For OP_CLOSE_ACCOUNT and other simple OK responses (no further args expected)
                             printf("%s operation successful.\n", op_name_str);
                        }
                    } else { // Not RESP_OK
                        char error_details[MAX_LINE_LEN] = "";
                        sscanf(recv_buffer, "%*s %255[^\n]", error_details);
                        printf("Server Error: %s %s\n", resp_status, error_details);
                    }
                } else {
                    printf("Failed to parse server response status: %s\n", recv_buffer);
                }
                wait_for_enter();
                break;
            }
            default:
                printf("Invalid choice. Please try again.\n");
                wait_for_enter();
        }
        if (client_socket_fd == -1 && choice !=0) { // check if connection was lost
             printf("Connection to server lost. Please restart the client.\n");
             break;
        }
    }
}


int main(int argc, char *argv[]) {
    const char* server_ip = "127.0.0.1"; // Default IP
    int server_port = SERVER_PORT;     // From common.h

    if (argc == 3) {
        server_ip = argv[1];
        server_port = atoi(argv[2]);
        if (server_port <= 0 || server_port > 65535) {
            fprintf(stderr, "Invalid port number: %s. Must be between 1 and 65535.\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    } else if (argc != 1) { // Allow 0 extra args (defaults) or 2 extra args (IP, Port)
        fprintf(stderr, "Usage: %s [server_ip server_port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    client_socket_fd = connect_to_server(server_ip, server_port);

    if (client_socket_fd == -1) {
        fprintf(stderr, "Failed to connect to the server at %s:%d.\n", server_ip, server_port);
        exit(EXIT_FAILURE);
    }

    handle_user_input(client_socket_fd);

    if (client_socket_fd != -1) { // If not already closed by an error in handle_user_input
        close(client_socket_fd);
        client_socket_fd = -1; // Mark as closed
    }
    printf("Disconnected from server.\n");
    return 0;
}
