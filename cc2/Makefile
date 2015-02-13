
OBJS = main.o parser.o cgen.o code.o optm.o

CPPFLAGS =
LDFLAGS = -L../lib
LIBS = -lcc

all: cc2

$(OBJS): ../inc/cc.h ../inc/sizes.h cc2.h
main.o: error.h

error.h: cc2.h
	./generror cc2.h > $@
			
cc2: $(OBJS) ../lib/libcc.a
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBS) -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2 error.h

