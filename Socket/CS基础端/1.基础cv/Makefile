CC := gcc
CFLAGS := -Wall -Wextra -std=c17

TARGETS := server client

.PHONY: all clean

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) $< -o $@

client: client.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
