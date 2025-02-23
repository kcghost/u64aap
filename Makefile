.PHONY=all

GIT_VERSION=$(shell git describe --always --dirty)
GIT_ORIGIN=$(shell git remote get-url origin)
CC=gcc
CFLAGS=-D_LANGUAGE_C -DGIT_VERSION=$(GIT_VERSION) -DGIT_ORIGIN=$(GIT_ORIGIN) -I ultralib/include

all: u64aap n64crc

u64aap: gopt.o vitbl.o u64aap.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

vitbl.o: ultralib/src/io/vitbl.c
	$(CC) $(CFLAGS) -c $< -o $@

n64crc: n64crc.o
	$(CC) $< -o $@

clean:
	rm -f u64aap
	rm -f n64crc
	rm -f *.o
