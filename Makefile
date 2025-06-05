CC = gcc
CFLAGS = -Wall -g -std=c11
LDFLAGS =

.PHONY: all clean

all: client server

client: client.c common.h
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

server: server.c common.h
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

clean:
	rm -f client server *.o
