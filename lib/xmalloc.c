
#include <stdlib.h>
#include <cc.h>

void *
xmalloc(size_t size)
{
	register void *p = malloc(size);

	if (!p)
		die("out of memory");
	return p;
}
