/* See LICENSE file for copyright and license details. */

#define __USE_MACROS
#include <ctype.h>
#undef isprint

int
isprint(int c)
{
	return __ctype[(unsigned char) c] & (_P|_U|_L|_D|_SP);
}
