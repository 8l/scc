include ../config.mk

OBJS = main.o parser.o cgen.o code.o optm.o peep.o

LIBS = -lcc

all: cc2

.POSIX:

$(OBJS): ../inc/cc.h ../inc/sizes.h cc2.h
main.o: error.h

error.h: cc2.h
	awk -f generror cc2.h > $@
			
cc2: $(OBJS) ../lib/libcc.a
	$(CC) -L../lib $(LDFLAGS) $(OBJS) $(LIBS) -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2 error.h

