
#include <string.h>
#include "../inc/cc.h"

char *
xstrdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *p = xmalloc(len);

	return memcpy(p, s, len);
}
