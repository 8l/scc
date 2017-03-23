/* See LICENSE file for copyright and license details. */

#define __USE_MACROS
#include <ctype.h>
#undef isalnum

int
isalnum(int c)
{
	return __ctype[(unsigned char) c] & (_U|_L|_D);
}
