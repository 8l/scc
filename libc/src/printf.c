/* See LICENSE file for copyright and license details. */

#include <stdarg.h>
#include <stdio.h>
#undef printf

int
printf(const char * restrict fmt, ...)
{
	int cnt;
	va_list va;

	va_start(va, fmt);
	cnt = vfprintf(stdin, fmt, va);
	va_end(va);
	return cnt;
}
