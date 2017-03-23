/* See LICENSE file for copyright and license details. */

#include <string.h>
#undef strxfrm

size_t
strxfrm(char * restrict dst, const char * restrict src, size_t n)
{
	size_t len = strlen(src);

	if (len < n)
		strcpy(dst, src);
	return len;
}
