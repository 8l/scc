
#include <assert.h>
#include <stdlib.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "symbol.h"

struct type tschar = {.op = CHAR};
struct type tshort = {.op = SHORT};
struct type tint = {.op = INT};
struct type tfloat = {.op = FLOAT};
struct type tdouble = {.op = DOUBLE};
struct type tldouble = {.op = LDOUBLE};
struct type tlong = {.op = LONG};
struct type tllong = {.op = LLONG};
struct type tvoid = {.op = VOID};
struct type tbool = {.op = BOOL};

static unsigned char stack[NR_DECLARATORS];
static unsigned char *stackp = stack;


static struct type *
mktype(register struct type *base, unsigned  char op)
{
	register struct type *nt;
	assert(op == PTR      || op == ARY	  || op == FTN ||
	       op == VOLATILE || op == RESTRICT   || op == CONST);

	nt = xcalloc(sizeof(*base), 1);
	nt->op = op;
	nt->base = base;
	return nt;
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

void ctype(struct ctype *cp, unsigned char mod)
{
	extern unsigned char nested_level;

	switch (mod) {
	case TYPEDEF:
		if (cp->c_type)
			goto duplicated;
		if (cp->c_extern | cp->c_auto | cp->c_reg | cp->c_static)
			goto two_storage;
		cp->c_type = 1;
		return;
	case EXTERN:
		if (cp->c_extern)
			goto duplicated;
		if (cp->c_type | cp->c_auto | cp->c_reg | cp->c_static)
			goto two_storage;
		cp->c_extern = 1;
		return;
	case STATIC:
		if (cp->c_static)
			goto duplicated;
		if (cp->c_type | cp->c_extern | cp->c_auto | cp->c_reg)
			goto two_storage;
		cp->c_static = 1;
		return;
	case AUTO:
		if (nested_level != 0)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extern | cp->c_static | cp->c_reg)
			goto two_storage;
		if (cp->c_auto)
			goto duplicated;
		cp->c_static = 1;
		return;
	case REGISTER:
		if (nested_level != 0)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extern | cp->c_auto | cp->c_static)
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
		if (user_opt.typeqlf_repeat && cp->c_volatile)
			goto duplicated;
		cp->c_volatile = 1;
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
	static const char *strings[] = {
		[ARY] =      "array of ",
		[PTR] =      "pointer to ",
		[FTN] =      "function that returns ",
		[VOLATILE] = "volatile ",
		[RESTRICT] = "restrict ",
		[CONST] =    "const ",
		[INT] =      "int ",
		[CHAR] =     "char ",
		[FLOAT] =    "float ",
		[LONG] =     "long ",
		[LLONG] =    "long long ",
		[SHORT] =    "short ",
		[VOID] =     "void ",
		[DOUBLE] =   "double ",
		[LDOUBLE] =  "long double "
	};
	assert(t);

	for (; t; t = t->base)
		fputs(strings[t->op], stdout);
	putchar('\n');
}

#endif
