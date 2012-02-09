
#include <assert.h>
#include <stdlib.h>

#include "tokens.h"
#include "types.h"


#define xcalloc calloc

struct type tschar = {.btype = CHAR, .sign = 1};
struct type tuchar = {.btype = CHAR, .sign = 0};
struct type tshort = {.btype = SHORT, .sign = 1};
struct type tushort = {.btype = SHORT, .sign = 0};
struct type tint = {.btype = INT, .sign = 1};
struct type tuint = {.btype = SHORT, .sign = 0};
struct type tfloat = {.btype = FLOAT, .sign = 1};
struct type tdouble = {.btype = DOUBLE, .sign = 1};
struct type tldouble = {.btype = LDOUBLE, .sign = 1};
struct type tlong = {.btype = LONG, .sign = 1};
struct type tulong = {.btype = LONG, .sign = 0};
struct type tllong = {.btype = LLONG, .sign = 1};
struct type tullong = {.btype = LLONG, .sign = 0};
struct type tvoid = {.btype = VOID, .sign = 0};

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
