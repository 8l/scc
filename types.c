
#include <assert.h>
#include <stdlib.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "symbol.h"


static unsigned char stack[NR_DECLARATORS];
static unsigned char *stackp = stack;


struct ctype *
newctype(void)
{
	register struct ctype *tp = xcalloc(sizeof(tp), 1);

	++tp->refcnt;
	return tp;
}

void
delctype(register struct ctype *tp)
{
	if (--tp->refcnt == 0) {
		if (tp->base)
			delctype(tp->base);
		free(tp);
	}
}

static struct ctype *
mktype(register struct ctype *tp, unsigned  char op)
{
	unsigned len;

	switch (op) {
	case ARY:
		assert(stackp != stack);
		len = *--stackp;
	case PTR: case FTN: {
		register struct ctype *aux = tp;

		++tp->refcnt;
		tp = newctype();
		tp->type = op;
		tp->base = aux;
		tp->len = len;
		break;
	}
	case VOLATILE:
		tp->c_volatile = 1;
		break;
	case RESTRICT:
		tp->c_restrict = 1;
		break;
	case CONST:
		tp->c_const = 1;
		break;
#ifndef NDEBUG
	default:
		abort();
#endif
	}
	return tp;
}

void
pushtype(unsigned mod)
{
	if (stackp == stack + NR_DECLARATORS)
		error("Too much type declarators");
	*stackp++ = mod;
}

struct ctype *
decl_type(struct ctype *tp)
{
	while (stackp != stack)
		tp = mktype(tp, *--stackp);
	return tp;
}

unsigned char
btype(unsigned char type, unsigned char tok)
{
	switch (tok) {
	case VOID:
		if (!type)
			return VOID;
		break;
	case BOOL:
		if (!type)
			return BOOL;
		break;
	case CHAR:
		if (!type)
			return CHAR;
		break;
	case SHORT:
		if (!type || type == INT)
			return SHORT;
		break;
	case INT:
		switch (type) {
		case 0:     return INT;
		case SHORT: return INT;
		case LONG:  return LONG;
		}
		break;
	case LONG:
		switch (type) {
		case 0: case INT:          return LONG;
		case LONG:                 return LLONG;
		case DOUBLE:               return LDOUBLE;
		case LLONG: case LDOUBLE:  error("too much long");
		}
		break;
	case FLOAT:
		if (!type)
			return FLOAT;
		break;
	case DOUBLE:
		if (!type)
			return DOUBLE;
		if (type == LONG)
			return LDOUBLE;
		break;
#ifndef NDEBUG
	default:
		abort();
#endif
	}
	error("two or more basic types");
}

void
storage(struct ctype *cp, unsigned char mod)
{
	extern unsigned char curctx;

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
		if (curctx != CTX_OUTER)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extern | cp->c_static | cp->c_reg)
			goto two_storage;
		if (cp->c_auto)
			goto duplicated;
		cp->c_static = 1;
		return;
	case REGISTER:
		if (curctx != CTX_OUTER)
			goto bad_file_scope_storage;
		if (cp->c_type | cp->c_extern | cp->c_auto | cp->c_static)
			goto two_storage;
		if (cp->c_reg)
			goto duplicated;
		cp->c_reg = 1;
		return;
	case CONST:
		if (options.repeat && cp->c_const)
			goto duplicated;
		cp->c_const = 1;
		return;
	case VOLATILE:
		if (options.repeat && cp->c_volatile)
			goto duplicated;
		cp->c_volatile = 1;
		return;
	}
bad_file_scope_storage:
	error("file-scope declaration specifies '%s'", yytext);
two_storage:
	error("Two or more storage specifier");
duplicated:
	error("duplicated '%s'", yytext);
}

#ifndef NDEBUG
#include <stdio.h>

void
ptype(register struct ctype *tp)
{
	static const char *strings[] = {
		[0] =        "[no type]",
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
	assert(tp);

	for (; tp; tp = tp->base)
		fputs(strings[tp->type], stdout);
	putchar('\n');
}

#endif
