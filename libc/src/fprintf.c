/* See LICENSE file for copyright and license details. */

#include <stdarg.h>
#include <stdio.h>
#undef fprintf

int
fprintf(FILE * restrict fp, const char * restrict fmt, ...)
{
	va_list va;
	int cnt;

	va_start(va, fmt);
	cnt = vfprintf(fp, fmt, va);
	va_end(va);
	return cnt;
}
