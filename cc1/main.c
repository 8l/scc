
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"

int warnings;
jmp_buf recover;

static char *output, *arg0;
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
	fprintf(stderr,
	        "usage: %s [-E] [-Dmacro[=value]] [-Idir] [-w] [-d] [-o output] [input]\n",
	        arg0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char c, *cp;

	atexit(clean);

	arg0 = (cp = strrchr(*argv, '/')) ? cp+1 : *argv;
	if (!strcmp(arg0, "cpp"))
		onlycpp = 1;

	for (;;) {
	nextiter:
		--argc, ++argv;
		if (!*argv || argv[0][0] != '-' || argv[0][1] == '-')
			break;
		for (cp = &argv[0][1]; (c = *cp); cp++) {
			switch (c) {
			case 'w':
				warnings = 1;
				break;
			case 'E':
				onlycpp = 1;
				break;
			case 'D':
				defmacro(cp+1);
				goto nextiter;
			case 'd':
				DBGON();
				break;
			case 'I':
				incdir(cp+1);
				goto nextiter;
			case 'o':
				if (!*++argv || argv[0][0] == '-')
					usage();
				--argc;
				output = *argv;
				break;
			default:
				usage();
			}
		}
	}

	if (output && !freopen(output, "w", stdout))
		die("error opening output:%s", strerror(errno));
	if (argc > 1)
		usage();

	icpp();
	ilex(*argv);

	if (onlycpp) {
		outcpp();
	} else {
		for (next(); yytoken != EOFTOK; decl())
			/* nothing */;
	}

	return 0;
}
