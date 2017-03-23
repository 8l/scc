/* See LICENSE file for copyright and license details. */

#define __USE_MACROS
#include <ctype.h>
#undef isxdigit

int
isxdigit(int c)
{
	return __ctype[(unsigned char) c] & (_D|_X);
}
