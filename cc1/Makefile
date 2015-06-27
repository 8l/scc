
OBJS = types.o decl.o lex.o error.o symbol.o main.o expr.o \
	code.o stmt.o cpp.o

all: cc1

$(OBJS) : cc1.h ../inc/cc.h ../inc/sizes.h

cc1: $(OBJS) ../lib/libcc.a
	$(CC) $(LDFLAGS) $(OBJS) ../lib/libcc.a -o $@

clean:
	rm -f $(OBJS)
	rm -f cc1

