/* See LICENSE file for copyright and license details. */

#include <ctype.h>
#include <stdlib.h>
#undef atoi

int
atoi(const char *s)
{
	int n, sign = -1;

	while(isspace(*s))
		++s;

	switch(*s) {
	case '-':
		sign = 1;
	case '+':
		++s;
	}

	/* Compute n as a negative number to avoid overflow on INT_MIN */
	for (n = 0; isdigit(*s); ++s)
		n = 10*n - (*s - '0');

	return sign * n;
}

