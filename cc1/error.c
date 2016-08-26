/* See LICENSE file for copyright and license details. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"

#define MAXERRNUM 10

extern int failure;
static unsigned nerrors;

static void
warn_error(int flag, char *fmt, va_list va)
{
	if (flag == 0)
		return;
	fprintf(stderr, "%s:%u: %s: ",
	       input->fname, input->nline,
	       (flag < 0) ? "error" : "warning");
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);

	if (flag < 0) {
		if (!failure)
			fclose(stdout);
		failure = 1;
		if (++nerrors == MAXERRNUM) {
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
	warn_error(warnings, fmt, va);
	va_end(va);
}

void
error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	warn_error(-1, fmt, va);
	va_end(va);
	exit(1);
	discard();
}

void
errorp(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	warn_error(-1, fmt, va);
	va_end(va);
}

void
cpperror(char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	warn_error(-1, fmt, va);
	va_end(va);

	/* discard input until the end of the line */
	*input->p = '\0';
	next();
}

void
unexpected(void)
{
	error("unexpected '%s'", yytext);
}
