/* See LICENSE file for copyright and license details. */

#define __USE_MACROS
#include <ctype.h>
#undef ispunct

int
ispunct(int c)
{
	return __ctype[(unsigned char) c] & (_P);
}
