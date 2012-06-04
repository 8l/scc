
#include <assert.h>
#include <stdlib.h>

#include "tokens.h"
#include "types.h"

/* TODO: create wrapper file */
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

#define TYPEOP_MAX PTRLEVEL_MAX /* TODO: take a look of the ANSI standard */

static unsigned char stack[TYPEOP_MAX];
static unsigned char *stackp = stack;


void pushtype(char op)
{
	if (stackp == stack + TYPEOP_MAX)
		error("Too much type modifiers");
	*stackp++ = op;
}

struct type *decl_type(struct type *t)
{
	while (stackp != stack)
		t = mktype(t, *--stackp);
	ptype(t);
	return t;
}

struct type *mktype(register struct type *base, unsigned  char op)
{
	register struct type **ptr, *nt;
	assert(op == PTR      || op == ARY	  || op == FTN ||
	       op == VOLATILE || op == RESTRICTED || op == CONST);

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
	case VOLATILE:
		ptr = &base->vltl;
		break;
	case RESTRICTED:
		ptr = &base->rstr;
		break;
	case CONST:
		ptr = &base->cnst;
		break;
	}
	if (*ptr)  return *ptr;

	nt = xcalloc(sizeof(*base), 1);
	*ptr = nt;
	nt->op = op;
	nt->base = base;
	return nt;
}




#ifndef NDEBUG
#include <stdio.h>

void ptype(register struct type *t)
{
	assert(t);

	for (; t; t = t->base) {
		switch (t->op) {
		case ARY:
			fputs("array of ", stdout);
			break;
		case PTR:
			fputs("pointer to ", stdout);
			break;
		case FTN:
			fputs("function that returns ", stdout);
			break;
		case VOLATILE:
			fputs("volatile ", stdout);
			break;
			break;
		case RESTRICTED:
			fputs("restricted ", stdout);
			break;
		case CONST:
			fputs("const ", stdout);
			break;
		default: {
				static char *type, *sign;

				sign = (t->sign) ? "signed" : "unsigned";
				switch (t->btype) {
				case INT:     type = "int";	    break;
				case CHAR:    type = "char";	    break;
				case FLOAT:   type = "float";       break;
				case LONG:    type = "long";	    break;
				case LLONG:   type = "long long";   break;
				case SHORT:   type = "short";       break;
				case VOID:    type = "void";	    break;
				case DOUBLE:  type = "double";      break;
				case LDOUBLE: type = "long double"; break;
				default:
					abort();
				}
				printf("%s %s", sign, type);
		}
		}
	}
	putchar('\n');
}

#endif


unsigned char isfunction(struct type *t)
{
	return t->op == FTN;
}
