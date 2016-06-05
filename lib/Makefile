# See LICENSE file for copyright and license details.
include ../config.mk

OBJS = die.o xcalloc.o xmalloc.o xrealloc.o xstrdup.o debug.o

all: libcc.a

.POSIX:

libcc.a: $(OBJS)
	ar r $@ $?

clean:
	rm -f *.o *.a
