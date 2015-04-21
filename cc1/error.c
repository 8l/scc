
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../inc/cc.h"
#include "cc1.h"

static void
warn_helper(int8_t flag, const char *fmt, va_list va)
{
	extern unsigned linenum;
	extern unsigned columnum;
	extern const char *filename;
	extern uint8_t failure;
	extern jmp_buf recover;

	if (!flag)
		return;
	fprintf(stderr, "%s:%s:%u: ",
		(flag < 0) ? "warning" : "error", filename, linenum);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	if (flag < 0) {
		failure = 1;
		longjmp(recover, 1);
	}
}

void
warn(const char *fmt, ...)
{
	extern uint8_t warnings;

	va_list va;
	va_start(va, fmt);
	warn_helper(warnings, fmt, va);
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

