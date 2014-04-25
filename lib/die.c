
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <cc.h>

void
die(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, fmt, va);
	va_end(va);
	exit(EXIT_FAILURE);
}
