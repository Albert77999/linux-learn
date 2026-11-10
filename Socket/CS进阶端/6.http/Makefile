CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g
LDLIBS := -pthread

.PHONY: all clean

all: server

server: server.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f server
