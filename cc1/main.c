
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../inc/cc.h"
#include "cc1.h"

int warnings;
jmp_buf recover;

static char *output;

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
	fputs("usage: cc1 [-w] [-o output] [input]\n", stderr);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char c, *cp;

	atexit(clean);

	for (;;) {
		--argc, ++argv;
		if (!*argv || argv[0][0] != '-' || argv[0][1] == '-')
			break;
		for (cp = &argv[0][1]; (c = *cp); cp++) {
			switch (c) {
			case 'w':
				warnings = 1;
				break;
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

	initcpp();
	ikeywords();

	if (!addinput(*argv, NULL))
		die("error opening input file '%s'", *argv);
	for (next(); yytoken != EOFTOK; extdecl())
		/* nothing */;

	return 0;
}
