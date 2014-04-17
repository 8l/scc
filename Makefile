
OBJS = types.o decl.o lex.o error.o symbol.o main.o expr.o \
	wrapper.o code.o stmt.o

all: kcc

$(OBJS) : cc.h

kcc: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f kcc

distclean: clean
	rm -f *~
	rm -f tags
	rm -f cscope.*
	rm -f makefile
