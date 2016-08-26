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

static char *name, *output;
static struct items uflags;
int onlycpp;

extern int failure;

static void
clean(void)
{
	if (failure && output)
		remove(output);
}

static void
defmacro(char *macro)
{
	char *p = strchr(macro, '=');

	if (p)
		*p++ = '\0';
	else
		p = "1";

	defdefine(macro, p, "command-line");
}

static void
usage(void)
{
	die(!strcmp(name, "cpp") ?
	    "usage: cpp [-wd] [-D def[=val]]... [-U def]... [-I dir]... "
	    "[input]" :
	    "usage: cc1 [-Ewd] [-D def[=val]]... [-U def]... [-I dir]... "
	    "[-o output] [input]");
}

int
main(int argc, char *argv[])
{
	char *cp;
	int i;

	atexit(clean);
	icpp();
	ilex();

	/* if run as cpp, only run the preprocessor */
	name = (cp = strrchr(*argv, '/')) ? cp + 1 : *argv;

	ARGBEGIN {
	case 'D':
		defmacro(EARGF(usage()));
		break;
	case 'E':
		onlycpp = 1;
		break;
	case 'I':
		incdir(EARGF(usage()));
		break;
	case 'U':
		newitem(&uflags, EARGF(usage()));
		break;
	case 'd':
		DBGON();
		break;
	case 'o':
		output = EARGF(usage());
		break;
	case 'w':
		warnings = 1;
		break;
	default:
		usage();
	} ARGEND

	if (argc > 1)
		usage();

	if (output && !freopen(output, "w", stdout))
		die("error opening output: %s", strerror(errno));

	if (!strcmp(name, "cpp"))
		onlycpp = 1;

	for (i = 0; i < uflags.n; ++i)
		undefmacro(uflags.s[i]);

	if (!addinput(*argv)) {
		die("error: failed to open input file '%s': %s",
		    *argv, strerror(errno));
	}
	if (onlycpp) {
		outcpp();
	} else {
		next();

		while (yytoken != EOFTOK)
			decl();
	}

	return failure;
}
