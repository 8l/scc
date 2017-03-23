/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strncat

char *
strncat(char * restrict dst, const char * restrict src, size_t n)
{
	char *ret = dst;

	while (*dst)
		++dst;
	while (n-- > 0 && *src)
		*dst++ = *src++;
	*dst = '\0';
	return ret;
}
