# See LICENSE file for copyright and license details.
.POSIX:

CSTDINC = -I../inc/$(STD)

include ../config.mk

OBJS = main.o parser.o peep.o symbol.o node.o code.o optm.o\
       arch/$(ARCH)/code.o arch/$(ARCH)/cgen.o \
       arch/$(ARCH)/types.o arch/$(ARCH)/optm.o

all: cc2-$(ARCH)

main.o: error.h
$(OBJS): cc2.h ../inc/$(STD)/cstd.h ../inc/cc.h
arch/$(ARCH)/code.o: arch/$(ARCH)/arch.h
arch/$(ARCH)/cgen.o: arch/$(ARCH)/arch.h

error.h: cc2.h
	rm -f $@; trap 'rm -f $$$$.h' EXIT INT QUIT ;\
	awk -f generror.awk cc2.h > $$$$.h && mv $$$$.h $@

../lib/libcc.a:
	cd ../lib && $(MAKE) -e

cc2-$(ARCH): $(OBJS) ../lib/libcc.a
	$(CC) $(SCC_LDFLAGS) $(OBJS) ../lib/libcc.a -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2-* error.h
