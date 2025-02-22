.PHONY=all

GIT_VERSION=$(shell git describe --always --dirty)
GIT_ORIGIN=$(shell git remote get-url origin)
CC=gcc
CFLAGS=-D_LANGUAGE_C -DGIT_VERSION=$(GIT_VERSION) -DGIT_ORIGIN=$(GIT_ORIGIN) -I ultralib/include

all: u64aap n64crc

u64aap: u64aap.c gopt.c gopt.h ultralib/src/io/vitbl.c
	$(CC) $(CFLAGS) u64aap.c gopt.c ultralib/src/io/vitbl.c -o $@

n64crc: n64crc.c
	$(CC) $< -o $@

clean:
	rm -f u64aap
	rm -f n64crc
