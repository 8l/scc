
OBJS = types.o decl.o lex.o error.o symbol.o main.o expr.o \
	wrapper.o

all: kcc

kcc: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS) -o $@

.PHONY: clean distclean dep

dep:
	(cat Makefile; \
	for i in $(OBJS:.o=.c); \
	do \
		$(CC) -MM $(CPPFLAGS) $$i; \
	done) > makefile

clean:
	rm -f $(OBJS)
	rm -f kcc

distclean: clean
	rm -f *~
	rm -f tags
	rm -f cscope.*
	rm -f makefile

###Dependencies
