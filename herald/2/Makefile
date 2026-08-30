# Max Rupplin - MEARVK LLC - 2026.
CC ?= cc
CFLAGS ?= -std=c90 -Wall -Wextra
all: herald2
herald2: level.c gate.c herald.c test.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f herald2
