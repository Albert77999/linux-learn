CC := gcc
CFLAGS := -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L

TARGETS := server client

.PHONY: all clean

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) $< -o $@

client: client.c
	$(CC) $(CFLAGS) $< -o $@

reference_server: server_reference.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
