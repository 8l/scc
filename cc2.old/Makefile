.POSIX:

include ../config.mk

OBJS = main.o parser.o cgen.o code.o optm.o peep.o

all: cc2


$(OBJS): ../inc/cc.h ../inc/sizes.h cc2.h
main.o: error.h

error.h: cc2.h
	rm -f $@; trap 'rm -f $$$$.h' EXIT INT QUIT
	awk -f generror cc2.h > $$$$.h && mv $$$$.h $@

cc2: $(OBJS) ../lib/libcc.a
	$(CC) $(LDFLAGS) $(OBJS) ../lib/libcc.a -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2 error.h

