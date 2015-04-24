
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

void
setsafe(uint8_t type)
{
	safe = type;
}

void
error(char *fmt, ...)
{
	int c;
	va_list va;
	extern FILE *yyin;

	va_start(va, fmt);
	warn_helper(-1, fmt, va);
	va_end(va);
	failure = 1;

	c = yytoken;
	do {
		switch (safe) {
		case END_COMP:
			if (c == '}')
				goto jump;
			goto semicolon;
		case END_COND:
			if (c == ')')
				goto jump;
			break;
		case END_LDECL:
			if (c == ',')
				goto jump;
		case END_DECL:
		semicolon:
			if (c == ';')
				goto jump;
			break;
		}
	} while ((c = getc(yyin)) != EOF);

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
