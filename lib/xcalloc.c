/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./lib/xcalloc.c";
#include <stdlib.h>
#include "../inc/cc.h"

void *
xcalloc(size_t n, size_t size)
{
	void *p = calloc(n, size);

	if (!p)
		die("out of memory");
	return p;
}
