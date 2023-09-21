CC=gcc
CFLAGS=-O2 -Wall

all: m65


m65: m65.o
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f m65.o gettab.o

tokens.h: gettab | mac65.bin
	./gettab < mac65.bin > tokens.h

mac65.bin:

m65.o: m65.c tokens.h
