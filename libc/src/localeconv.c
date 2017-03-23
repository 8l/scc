#include <locale.h>
#include <limits.h>
#undef localeconv

struct lconv *
localeconv(void)
{
	static struct lconv lc = { ".", "", "", "", "", "", "", "", "", "",
	                           CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
	                           CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
	                           CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
	                           CHAR_MAX, CHAR_MAX };
	return &lc;
}
