CC = gcc
CFLAGS = -Wall -g
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SERVER_SRC = $(SRC_DIR)/server.c
CLIENT_SRC = $(SRC_DIR)/client.c
UTILS_SRC = $(SRC_DIR)/utils.c

# Object files
SERVER_OBJ = $(OBJ_DIR)/server.o
CLIENT_OBJ = $(OBJ_DIR)/client.o
UTILS_OBJ = $(OBJ_DIR)/utils.o

# Executables
SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client

# Default target
all: $(SERVER_BIN) $(CLIENT_BIN)

# Create directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build server
$(SERVER_BIN): $(SERVER_OBJ) $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build client
$(CLIENT_BIN): $(CLIENT_OBJ) $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile server source
$(OBJ_DIR)/server.o: $(SERVER_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile client source
$(OBJ_DIR)/client.o: $(CLIENT_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile utils source
$(OBJ_DIR)/utils.o: $(UTILS_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

.PHONY: all clean