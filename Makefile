CC=gcc
CFLAGS=-O2 -Wall

all: m65


m65: m65.o
        
m65.o: m65.c tokens.h

tokens.h: gettab mac65.bin
	./gettab < mac65.bin > tokens.h
