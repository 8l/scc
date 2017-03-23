/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strchr

char *
strchr(const char *s, int c)
{
	while (*s && *s != c)
		++s;
	return (*s == c) ? (char *)s : NULL;
}
