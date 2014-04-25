
OBJS = types.o decl.o lex.o error.o symbol.o main.o expr.o \
	wrapper.o code.o stmt.o

CFLAGS += -I../include

all: cc1

$(OBJS) : cc1.h

cc1: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS) -o $@

clean:
	rm -f $(OBJS)
	rm -f cc1

distclean: clean
	rm -f *~
	rm -f tags
	rm -f cscope.*
	rm -f makefile
