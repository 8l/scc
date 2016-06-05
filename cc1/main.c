/* See LICENSE file for copyright and license details. */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../inc/arg.h"
#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"

char *argv0;

int warnings;
jmp_buf recover;

static char *output;
int onlycpp;

static void
clean(void)
{
	extern int failure;

	if (failure && output)
		remove(output);
}

static void
usage(void)
{
	die("usage: %s [-E] [-D macro[=value]] ... [-I dir] [-w] [-d]"
	    "[-o output] [input]", argv0);
}

int
main(int argc, char *argv[])
{
	char *base;

	atexit(clean);

	ARGBEGIN {
	case 'w':
		warnings = 1;
		break;
	case 'E':
		onlycpp = 1;
		break;
	case 'D':
		defmacro(EARGF(usage()));
		break;
	case 'd':
		DBGON();
		break;
	case 'I':
		incdir(EARGF(usage()));
		break;
	case 'o':
		output = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (argc > 1)
		usage();

	/* if run as cpp, only run the preprocessor */
	if ((base = strrchr(argv0, '/')))
		++base;
	else
		base = argv0;
	if (!strcmp(base, "cpp"))
		onlycpp = 1;

	if (output && !freopen(output, "w", stdout))
		die("error opening output: %s", strerror(errno));

	icpp();
	ilex(*argv);

	if (onlycpp) {
		outcpp();
	} else {
		next();

		while (yytoken != EOFTOK)
			decl();
	}

	return 0;
}
