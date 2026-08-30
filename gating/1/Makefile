# Max Rupplin - MEARVK LLC - 2026.
CC ?= cc
CFLAGS ?= -std=c90 -Wall -Wextra
all: gate1

gate1: level.c gate.c herald.c test.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f gate1
