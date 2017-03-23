/* See LICENSE file for copyright and license details. */

#define __USE_MACROS
#include <ctype.h>
#undef toupper

int
toupper(int c)
{
	return (islower(c)) ? c & ~0x20 : c;
}
