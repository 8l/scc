
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../inc/cc.h"
#include "cc1.h"

extern void init_keywords(void),
	open_file(const char *file),  init_expr(void);

uint8_t npromote, warnings;
jmp_buf recover;

static char *output;

static void
clean(void)
{
	extern uint8_t failure;

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
	char c, *input, *cp;

	atexit(clean);

repeat:
	--argc, ++argv;
	if (*argv && argv[0][0] == '-' && argv[0][1] != '-') {
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
		goto repeat;
	}

	if (output) {
		if (!freopen(output, "w", stdout))
			die("error opening output:%s", strerror(errno));
	}
	input = *argv;
	if (argc > 1)
		usage();
	init_keywords();
	init_expr();
	open_file(input);

	setjmp(recover);
	next();

	while (yytoken != EOFTOK)
		extdecl();

	if (fclose(stdin))
		die("error reading from input:%s", strerror(errno));
	if (fclose(stdout))
		die("error writing in output:%s", strerror(errno));
	return 0;
}
