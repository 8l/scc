/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#undef rand

static int next;

void
srand(unsigned seed)
{
	next = seed;
}

int
rand(void)  /* RAND_MAX assumed to be 32767. */
{
	next = next * 1103515245 + 12345;
	return (unsigned)(next/65536) % 32768;
}
