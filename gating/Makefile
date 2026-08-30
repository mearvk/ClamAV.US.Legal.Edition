# Max Rupplin - MEARVK LLC - 2026.
CC ?= cc
CFLAGS ?= -std=c90 -Wall -Wextra
LEVELS=1 2 3 4 5 6 7 8
all:
	@for n in $(LEVELS); do d=`if [ $$((n%2)) -eq 1 ]; then echo gating/$$n; else echo herald/$$n; fi`; echo "building $$d"; $(MAKE) -C $$d; done
clean:
	@for n in $(LEVELS); do d=`if [ $$((n%2)) -eq 1 ]; then echo gating/$$n; else echo herald/$$n; fi`; $(MAKE) -C $$d clean; done
