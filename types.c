
#include <assert.h>
#include <stdlib.h>

#include "types.h"

#define xcalloc calloc

struct type tschar, tuchar;             /* signed char, unsigned char */
struct type tshort, tushort;            /* short,  unsigned short */
struct type tint, tuint;                /* int,  unsigned int */
struct type tfloat, tdouble, tldouble;	/* float, double, long double */
struct type tlong, tulong;		/* long, unsgined long */
struct type tllong, tullong;		/* long long, unsigned long long */
struct type tvoid;			/* void */


struct type *mktype(register struct type *base, unsigned  char op)
{
	register struct type **ptr, *nt;
	assert(op == PTR || op == ARY || op == FTN);

	switch (op) {
	case PTR:
		ptr = &base->ptr;
		break;
	case ARY:
		ptr = &base->ary;
		break;
	case FTN:
		ptr = &base->ftn;
		break;
	}
	if (*ptr)  return *ptr;

	nt = xcalloc(sizeof(*base), 1);
	*ptr = nt;
	nt->op = op;
	nt->base = base;
	return nt;
}
