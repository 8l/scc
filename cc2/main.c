
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "arch.h"
#include "cc2.h"
#include "error.h"

void
error(unsigned nerror, ...)
{
	va_list va;
	va_start(va, nerror);
	vfprintf(stderr, errlist[nerror], va);
	va_end(va);
	putc('\n', stderr);
	exit(1);
}

static int
moreinput(void)
{
	int c;

repeat:
	if (feof(stdin))
		return 0;
	if ((c = getchar()) == '\n' || c == EOF)
		goto repeat;
	ungetc(c, stdin);
	return 1;
}

int
main(void)
{

	while (moreinput()) {
		parse();
		apply(optm);
		apply(sethi);
		apply(cgen);
		peephole();
		writeout();
	}
	return 0;
}
