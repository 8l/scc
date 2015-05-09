
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../inc/cc.h"
#include "cc1.h"

#define MAXERRNUM 10

extern uint8_t failure;
static uint8_t nerrors;

static void
warn_helper(int8_t flag, char *fmt, va_list va)
{
	if (!flag)
		return;
	fprintf(stderr, "%s:%s:%u: ",
		(flag < 0) ? "error" : "warning", filename(), fileline());
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	if (flag < 0 && nerrors++ == MAXERRNUM) {
		fputs("too many errors\n", stderr);
		exit(-1);
	}
}

void
warn(char *fmt, ...)
{
	extern uint8_t warnings;

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
	failure = 1;
	discard();
}

void
softerror(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	warn_helper(-1, fmt, va);
	va_end(va);
	failure = 1;
}

void
unexpected(void)
{	
	error("unexpected '%s'", yytext);
}
