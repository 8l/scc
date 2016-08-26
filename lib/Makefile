# See LICENSE file for copyright and license details.
.POSIX:
include ../config.mk

OBJS = debug.o die.o newitem.o xcalloc.o xmalloc.o xrealloc.o xstrdup.o

all: libcc.a


libcc.a: $(OBJS)
	ar $(ARFLAGS) $@ $?
	ranlib $@

clean:
	rm -f *.o *.a
