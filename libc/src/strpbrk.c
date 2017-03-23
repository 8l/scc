/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strpbrk

char *
strpbrk(const char *s1, const char *s2)
{
	int c;
	const char *p;

	for (; c = *s1; ++s1) {
		for (p = s2; *p && *p != c; ++p)
			/* nothing */;
		if (*p == c)
			return s1;
	}
	return NULL;
}
