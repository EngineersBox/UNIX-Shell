# Feel free to extend this Makefile as you see fit. Just ensure that
# running `make` will compile your code to a binary named `anubis`.

CC=gcc
# Release
CFLAGS=-Wall
# Debug
#CFLAGS=-O0 -Wall -lm -g -fno-omit-frame-pointer

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

anubis: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) anubis $(OBJS)

.PHONY: clean
