
#include <stdlib.h>
#include <string.h>

#include "cc.h"


static void
out_of_memory(void)
{
	/* TODO: deal with out of memory errors */
	error("out of memory");
}

void *
xmalloc(size_t size)
{
	register void *p = malloc(size);

	if (!p)
		out_of_memory();
	return p;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	register size_t nbytes = nmemb * size;
	register void *p = xmalloc(nbytes);

	return memset(p, nbytes, 0);
}

char *
xstrdup(const char *s)
{
	register size_t len = strlen(s);
	register char *p = xmalloc(len);

	return memcpy(p, s, len);
}

void *
xrealloc(void *buff, register size_t size)
{
	register void *p = realloc(buff, size);

	if (!p)
		out_of_memory();
	return p;
}
