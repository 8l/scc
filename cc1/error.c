
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../inc/cc.h"
#include "cc1.h"

#define MAXERRNUM 10

extern int failure;
static unsigned nerrors;

static void
warn_helper(int flag, char *fmt, va_list va)
{
	if (flag == 0)
		return;
	fprintf(stderr, "%s:%u: %s: ",
	       input->fname, input->nline,
	       (flag < 0) ? "error" : "warning");
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);

	if (flag < 0) {
		failure = 1;
		if (nerrors++ == MAXERRNUM) {
			fputs("too many errors\n", stderr);
			exit(1);
		}
	}
}

void
warn(char *fmt, ...)
{
	extern int warnings;

	va_list va;
	va_start(va, fmt);
	warn_helper(warnings, fmt, va);
	va_end(va);
}

void
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	warn_helper(-1, fmt, va);
	va_end(va);
	exit(1);
	discard();
}

void
printerr(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	warn_helper(-1, fmt, va);
	va_end(va);
}

void
unexpected(void)
{
	error("unexpected '%s'", yytext);
}
