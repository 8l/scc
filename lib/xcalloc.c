
#include <stdlib.h>
#include <cc.h>

void *
xcalloc(size_t n, size_t size)
{
	register void *p = calloc(n, size);

	if (!p)
		die("out of memory");
	return p;
}
