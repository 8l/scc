
#include <stdlib.h>
#include "../inc/cc.h"

void *
xrealloc(void *buff, size_t size)
{
	void *p = realloc(buff, size);

	if (!p)
		die("out of memory");
	return p;
}
