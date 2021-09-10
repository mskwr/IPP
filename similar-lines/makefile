# Makefile małego zadania z IPP
# Autor: Michał Skwarek

PROGRAM  = similar_lines
CC       = gcc
CPPFLAGS =
CFLAGS   = -Wall -Wextra -std=c11 -O2
LDFLAGS  =

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): main.o recognizer.o parser.o similar.o
	$(CC) $(CFLAGS) -o $@ $^

recognizer.o: recognizer.c recognizer.h multiset.h
	$(CC) $(CFLAGS) -c $<

parser.o : parser.c parser.h recognizer.h multiset.h
	$(CC) $(CFLAGS) -c $<

similar.o: similar.c similar.h multiset.h
	$(CC) $(CFLAGS) -c $<

main.o: main.c parser.h similar.h multiset.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o similar_lines
