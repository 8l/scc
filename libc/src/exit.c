/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#undef exit

void (*_exitf[_ATEXIT_MAX])(void);
unsigned _exitn;

void
exit(int status)
{
	while (_exitn > 0)
		(*_exitf[--_exitn])();
	_Exit(status);
}
