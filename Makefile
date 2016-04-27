# Makefile for virtual memory program
CC=gcc
CFLAGS=-Wall

all: virtual_memory

vm: virtual_memory.o
	$(CC) $(CFLAGS) -o virtual_memory virtual_memory.o
