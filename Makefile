CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: bingo

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

bingo.o: bingo.c csapp.h
	$(CC) $(CFLAGS) -c bingo.c

bingo: bingo.o csapp.o
	$(CC) $(CFLAGS) bingo.o csapp.o -o bingo $(LDFLAGS)
