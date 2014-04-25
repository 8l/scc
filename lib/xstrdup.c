
#include <string.h>
#include <cc.h>

char *
xstrdup(const char *s)
{
	register size_t len = strlen(s) + 1;
	register char *p = xmalloc(len);

	return memcpy(p, s, len);
}
