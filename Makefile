
OBJS = types.o decl.o lex.o error.o symbol.o flow.o main.o
LIBS =

all: kcc

kcc: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS) -o $@

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< | \
	sed -e 's,/usr/[^ ]*,,g' | \
	egrep -v '^ *\\$$' > $@

.PHONY: clean distclean dep

dep: $(OBJS:.o=.d)
	cat Makefile $? > makefile
	rm -f *.d

clean:
	rm -f $(OBJS)
	rm -f cc

distclean: clean
	rm -f *~
	rm -f *.d
	rm -f makefile



###Dependencies
