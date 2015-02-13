
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "../inc/cc.h"
#include "cc1.h"

extern unsigned linenum;
extern unsigned columnum;
extern const char *filename;

static void
warn_helper(signed char flag, const char *fmt, va_list va)
{
	if (!flag)
		return;
	fprintf(stderr, "%s:%s:%u: ",
		(!flag) ? "warning" : "error", filename, linenum);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	if (flag < 0)
		exit(EXIT_FAILURE); /* TODO: uhmmmm */
}

void
warn(signed char flag, const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	warn_helper(flag, fmt, va);
	va_end(va);
}

void
error(const char *fmt, ...)
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

