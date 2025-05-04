CC = gcc
CFLAGS = -Os
LIBS = -lcurl

all: server

server: main.c
	$(CC) $(CFLAGS) main.c -o server $(LIBS)