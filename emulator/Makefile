CC = gcc
CCFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Iinclude

CFILES = $(shell find . -type f -name "*.c")
OUTFILE = kemu

all:
	$(CC) $(CFILES) -o $(OUTFILE) $(CCFLAGS)
