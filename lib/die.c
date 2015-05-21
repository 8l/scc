
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "../inc/cc.h"

int failure;

void
die(const char *fmt, ...)
{
	failure = 1;
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, fmt, va);
	va_end(va);
	exit(EXIT_FAILURE);
}
