CC=gcc
CFLAGS=-lX11
LDFLAGS=

.PHONY: clean all

all: paths build

paths:
	mkdir -p bin
build:
	$(CC) main.c $(CFLAGS) -o bin/msv3
