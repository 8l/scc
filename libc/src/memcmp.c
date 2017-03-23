/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef memcmp

int
memcmp(const void *s1, const void *s2, size_t n)
{
	char *s = (char *) s1, *t = (char *) s2;

	while (n > 0 && *s == *t)
		--n, ++s, ++t;
	return n ? (*s - *t) : 0;
}
