
#include <assert.h>
#include <stdlib.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "symbol.h"

/* TODO: create wrapper file */
#define xcalloc calloc

struct type tschar = {.btype = CHAR};
struct type tshort = {.btype = SHORT};
struct type tint = {.btype = INT};
struct type tfloat = {.btype = FLOAT};
struct type tdouble = {.btype = DOUBLE};
struct type tldouble = {.btype = LDOUBLE};
struct type tlong = {.btype = LONG};
struct type tllong = {.btype = LLONG};
struct type tvoid = {.btype = VOID};
struct type tbool = {.btype = BOOL};

static unsigned char stack[NR_DECLARATORS];
static unsigned char *stackp = stack;

struct type *btype(struct type *tp, unsigned char tok)
{
	switch (tok) {
	case VOID:
		if (tp == NULL)
			return T_VOID;
		break;
	case BOOL:
		if (tp == NULL)
			return T_BOOL;
		break;
	case CHAR:
		if (tp == NULL)
			return T_CHAR;
		break;
	case SHORT:
		if (tp == NULL || tp == T_INT)
			return T_SHORT;
		break;
	case INT:
		if (tp == NULL)
			return T_INT;
		if (tp == T_SHORT)
			return T_SHORT;
		if (tp == T_LONG)
			return T_LONG;
		break;
	case LONG:
		if (tp == NULL || tp == T_INT)
			return T_LONG;
		if (tp == T_LONG)
			return T_LLONG;
		if (tp == T_DOUBLE)
			return T_LDOUBLE;
		if (tp == T_LLONG)
			error("'long long long' is too long");
		if (tp == T_LDOUBLE)
			error("'long long double' is too long");
		break;
	case FLOAT:
		if (tp == NULL)
			return T_FLOAT;
		break;
	case DOUBLE:
		if (tp == NULL)
			return T_DOUBLE;
		if (tp == T_LONG)
			return T_LDOUBLE;
		break;
	default:
		abort();
	}
	error("two or more basic types");
}

void pushtype(unsigned char mod)
{
	if (stackp == stack + NR_DECLARATORS)
		error("Too much type declarators");
	*stackp++ = mod;
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
	       op == VOLATILE || op == RESTRICT   || op == CONST);

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
	case RESTRICT:
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

void ctype(struct ctype *cp, unsigned char mod)
{
	extern unsigned char nested_level;

	switch (mod) {
	case TYPEDEF:
		if (cp->c_type)
			goto duplicated;
		if (cp->c_extrn | cp->c_auto | cp->c_reg | cp->c_static)
			goto two_storage;
		cp->c_type = 1;
		return;
	case EXTERN:
		if (cp->c_extrn)
			goto duplicated;
		if (cp->c_type | cp->c_auto | cp->c_reg | cp->c_static)
			goto two_storage;
		cp->c_extrn = 1;
		return;
	case STATIC:
		if (cp->c_static)
			goto duplicated;
		if (cp->c_type | cp->c_extrn | cp->c_auto | cp->c_reg)
			goto two_storage;
		cp->c_static = 1;
		return;
	case AUTO:
		if (nested_level != 0)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extrn | cp->c_static | cp->c_reg)
			goto two_storage;
		if (cp->c_auto)
			goto duplicated;
		cp->c_static = 1;
		return;
	case REGISTER:
		if (nested_level != 0)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extrn | cp->c_auto | cp->c_static)
			goto two_storage;
		if (cp->c_reg)
			goto duplicated;
		cp->c_reg = 1;
		return;
	case CONST:
		if (user_opt.typeqlf_repeat && cp->c_reg)
			goto duplicated;
		cp->c_const = 1;
		return;
	case VOLATILE:
		if (user_opt.typeqlf_repeat && cp->c_vol)
			goto duplicated;
		cp->c_vol = 1;
		return;
	}
bad_file_scope_storage:
	error("file-scope declaration specifies ‘%s’", yytext);
two_storage:
	error("Two or more storage specifier");
duplicated:
	error("duplicated '%s'", yytext);
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
		case RESTRICT:
			fputs("restrict ", stdout);
			break;
		case CONST:
			fputs("const ", stdout);
			break;
		default: {
				static char *type, *sign;

				/* sign = (t->sign) ? "signed" : "unsigned"; */
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
				/* printf("%s %s", sign, type); */
		}
		}
	}
	putchar('\n');
}

#endif
