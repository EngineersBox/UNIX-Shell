# Feel free to extend this Makefile as you see fit. Just ensure that
# running `make` will compile your code to a binary named `anubis`.

CC = gcc
CFLAGS = -Wall
LIBS=

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

anubis: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^ ${LIBS}

clean:
	$(RM) anubis $(OBJS) 

.PHONY: clean
