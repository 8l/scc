/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strncmp

int
strncmp(const char *s1, const char *s2, size_t n)
{
	for (; n && *s1 && *s2 && *s1 == *s2; --n, ++s1, ++s2);
		/* nothing */;
	return n ? (*(unsigned char *)s1 - *(unsigned char *)s2) : 0;
}
