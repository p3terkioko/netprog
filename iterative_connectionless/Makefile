CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = 

all: server client

server: server.c bank_utils.c
	$(CC) $(CFLAGS) -o server server.c bank_utils.c $(LDFLAGS)

client: client.c bank_utils.c
	$(CC) $(CFLAGS) -o client client.c bank_utils.c $(LDFLAGS)

clean:
	rm -f server client 