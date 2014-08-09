
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cc.h>

#include "cc2.h"
#include "error.h"

extern void parse(void);

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
	exit(EXIT_FAILURE);
}

int
main(void)
{
	parse();
}
