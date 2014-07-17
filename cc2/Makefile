
OBJS = main.o parser.o

CPPFLAGS = -I../inc
LDFLAGS = -L../lib
LIBS = -lcc

all: cc2

$(OBJS): ../inc/cc.h ../inc/sizes.h

cc2: $(OBJS) ../lib/libcc.a
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBS) -o $@

clean:
	rm -f $(OBJS)
	rm -f cc2

