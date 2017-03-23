/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef memchr

void *
memchr(const void *s, int c, size_t n)
{
	unsigned char *bp = (char *) s;

	while (n > 0 && *bp++ != c)
		--n;
	return (n == 0) ? NULL : bp-1;
}
