.POSIX:

all: tests

tests:
	CFLAGS='' SCCEXECPATH=../../bin PATH=../../bin:$$PATH ./chktest.sh < scc-tests.lst
clean:
	rm -f *.as *.o *.ir *.qbe *core test.log
