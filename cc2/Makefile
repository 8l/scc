.POSIX:

include ../config.mk

OBJS = main.o parser.o optm.o peep.o symbol.o node.o code.o\
       arch/$(ARCH)/code.o arch/$(ARCH)/cgen.o arch/$(ARCH)/types.o

all: cc2


$(OBJS): cc2.h
main.o: error.h

error.h: cc2.h
	rm -f $@; trap 'rm -f $$$$.h' EXIT INT QUIT ;\
	awk -f generror cc2.h > $$$$.h && mv $$$$.h $@

cc2: $(OBJS) ../lib/libcc.a
	$(CC) $(SCC_LDFLAGS) $(OBJS) ../lib/libcc.a -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2 error.h
