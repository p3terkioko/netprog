CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread
TARGETS = server client

all: $(TARGETS)

server: server.o common.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o common.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean