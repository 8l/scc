/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strcspn

size_t
strcspn(const char *s1, const char *s2)
{
	size_t n;
	int c;
	const char *p;

	for (n = 0; c = *s1++; ++n) {
		for (p = s2; *p && *p != c; ++p)
			/* nothing */;
		if (*p == c)
			break;
	}
	return n;
}
