CC=gcc

RM=rm

CFLAGS=-c

LDFLAGS=-lm

SOURCES=$(wildcard *.c)

OBJECTS=$(SOURCES:.c=.o)

EXECS=$(SOURCES:%.c=%)

.PHONY: all
all: $(OBJECTS)
	gcc -o main parsecmd.o main.o

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.o.: 
	$(CC) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	-@ $(RM) *.o 
	rm main