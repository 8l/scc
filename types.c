
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "symbol.h"


static unsigned char stack[NR_DECLARATORS];
static unsigned char *stackp = stack;

struct ctype *
initctype(register struct ctype *tp)
{
	memset(tp, 0, sizeof(*tp));
	tp->forward = 1;
	return tp;
}

void
delctype(register struct ctype *tp)
{
	if (!tp)
		return;
	if (tp->base)
		delctype(tp->base);
	free(tp);
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

		tp = xmalloc(sizeof(*tp));
		initctype(tp);
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
	default:
		assert(0);
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
	struct ctype *new = xmalloc(sizeof(*new));
	*new = *tp;

	while (stackp != stack)
		new = mktype(new, *--stackp);
	return new;
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
