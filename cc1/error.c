
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../inc/cc.h"
#include "cc1.h"

extern uint8_t failure;
extern jmp_buf recover;

static uint8_t safe;

static void
warn_helper(int8_t flag, char *fmt, va_list va)
{
	extern unsigned linenum;
	extern unsigned columnum;
	extern const char *filename;

	if (!flag)
		return;
	fprintf(stderr, "%s:%s:%u: ",
		(flag < 0) ? "error" : "warning", filename, linenum);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
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

bool
setsafe(uint8_t type)
{
	safe = type;
	if (setjmp(recover))
		return 0;
	return 1;
}

void
error(char *fmt, ...)
{
	int c;
	va_list va;

	va_start(va, fmt);
	warn_helper(-1, fmt, va);
	va_end(va);
	failure = 1;

	c = yytoken;
	do {
		switch (safe) {
		case END_DECL:
			if (c == ';')
				goto jump;
			break;
		}
	} while ((c = getchar()) != EOF);

jump:
	yytoken = c;
	longjmp(recover, 1);
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
