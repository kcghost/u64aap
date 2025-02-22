.PHONY=all

CC=gcc
CFLAGS=-D_LANGUAGE_C -I ultralib/include

all: u64aap

u64aap: u64aap.c gopt.c gopt.h ultralib/src/io/vitbl.c
	$(CC) $(CFLAGS) u64aap.c gopt.c ultralib/src/io/vitbl.c -o $@

clean:
	rm -f u64aap
