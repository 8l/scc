/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../inc/cc.h"
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
main(int argc, char *argv[])
{
	if (argc > 2)
		die("usage: cc2 [irfile]");

	if (argv[1]) {
		if (!freopen(argv[1], "r", stdin))
			die("cc2: cannot open %s", argv[1]);
	}

	while (moreinput()) {
		parse();
		apply(optm_ind);
		apply(optm_dep);
		apply(sethi);
		apply(cgen);
		peephole();
		writeout();
	}
	return 0;
}
