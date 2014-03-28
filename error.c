
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "cc.h"




static void
warn_helper(char flag, const char *fmt, va_list va)
{
	fprintf(stderr, "%s:%s:%u:%u: ",
		(!flag) ? "warning" : "error", filename, linenum, columnum);
	vfprintf(stderr, fmt, va);
	putc('\n', stderr);
	if (flag)
		exit(EXIT_FAILURE); /* TODO: uhmmmm */
}

void
warn(char flag, const char *fmt, ...)
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
	warn_helper(1, fmt, va);
	va_end(va);
}

void
die(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, fmt, va);
	va_end(va);
	exit(EXIT_FAILURE);
}
