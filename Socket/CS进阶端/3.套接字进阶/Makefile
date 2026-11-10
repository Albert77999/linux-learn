CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g

TARGETS := stdcpy syscpy server client

.PHONY: all test test-std test-sys clean

all: $(TARGETS)

stdcpy: stdcpy.c
	$(CC) $(CFLAGS) $< -o $@

syscpy: syscpy.c
	$(CC) $(CFLAGS) $< -o $@

server: server.c
	$(CC) $(CFLAGS) $< -o $@

client: client.c
	$(CC) $(CFLAGS) $< -o $@

test: test-std test-sys

test-std: stdcpy news.txt
	./stdcpy
	cmp news.txt cpy.txt
	@echo "stdcpy: copy succeeded"

test-sys: syscpy news.txt
	./syscpy
	cmp news.txt cpy.txt
	@echo "syscpy: copy succeeded"

clean:
	rm -f $(TARGETS) cpy.txt
