/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <errno.h>
#undef atexit

extern void (*_exitf[_ATEXIT_MAX])(void);
extern unsigned _exitn;

int
atexit(void (*fun)(void))
{
	if (_exitn == _ATEXIT_MAX) {
		errno = ENOMEN;
		return -1;
	}
	_exitf[_exitn++] = fun;
	return 0;
}
