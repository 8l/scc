/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strncpy

char *
strncpy(char * restrict dst, const char * restrict src, size_t n)
{
	char *ret = dst;

	for (; n > 0 && *src; --n)
		*dst++ = *src++;
	while (n-- > 0)
		*dst++ = '\0';
	return ret;
}
