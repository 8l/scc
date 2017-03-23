/* See LICENSE file for copyright and license details. */

#include <locale.h>
#include <stddef.h>
#undef setlocale

char *
setlocale(int category, const char *locale)
{
	if (category > LC_TIME || category < LC_ALL)
		return NULL;
	if (!locale ||
	    locale[0] == '\0' ||
	    locale[0] == 'C' && locale[1] == '\0') {
		return "C";
	}
	return NULL;
}
