/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strstr

char *
strstr(const char *s1, const char *s2)
{
	const char *p, *q;
	int c;

	c = *s2++;
	if (c == '\0')
		return (char *) s1;

	while (*s1) {
		if (*s1 != c) {
			++s1;
		} else {
			p = s1++;
			for (q = s2; *q && *s1 == *q; ++s1, ++q)
				/* nothing */;
			if (*q == '\0')
				return (char *) p;
		}
	}
	return NULL;
}
