# See LICENSE file for copyright and license details.
.POSIX:

include ../config.mk

OBJS = main.o parser.o peep.o symbol.o node.o code.o optm.o\
       arch/$(ARCH)/code.o arch/$(ARCH)/cgen.o \
       arch/$(ARCH)/types.o arch/$(ARCH)/optm.o

all: cc2

error.h: cc2.h
	rm -f $@; trap 'rm -f $$$$.h' EXIT INT QUIT ;\
	awk -f generror.awk cc2.h > $$$$.h && mv $$$$.h $@

main.o: error.h

../inc/sizes.h:
	cp ../inc/sizes_$(STD).h $@

$(OBJS): cc2.h ../inc/sizes.h

../lib/libcc.a:
	cd ../lib && $(MAKE) -e

cc2: $(OBJS) ../lib/libcc.a
	$(CC) $(SCC_LDFLAGS) $(OBJS) ../lib/libcc.a -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2 error.h
