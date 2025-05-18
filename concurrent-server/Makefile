# Makefile

CC = gcc
CFLAGS = -Wall
SRC = src/server.c src/client.c src/bankv1.c
OBJ = $(SRC:.c=.o)
TARGET_SERVER = server
TARGET_CLIENT = client

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): src/server.o src/bankv1.o
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) src/server.o src/bankv1.o

$(TARGET_CLIENT): src/client.o
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) src/client.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET_SERVER) $(TARGET_CLIENT)