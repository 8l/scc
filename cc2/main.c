
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "../inc/cc.h"

#include "cc2.h"
#include "error.h"

char odebug;

void
error(unsigned nerror, ...)
{
	va_list va;
	va_start(va, nerror);
	if (nerror >= ENUMERR)
		fprintf(stderr, "incorrect error number '%d'", nerror);
	else
		vfprintf(stderr, errlist[nerror], va);
	va_end(va);
	putc('\n', stderr);
	exit(1);
}

bool
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
	fputs("cc2 is not updated with the output of cc1", stderr);
	exit(1);
	while (moreinput()) {
		parse();
		optimize();
		addable();
		generate();
		peephole();
		writeout();
	}
	return 0;
}
