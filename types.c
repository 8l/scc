
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
	register struct ctype *tp = xcalloc(sizeof(*tp), 1);

	tp->refcnt = 1;
	return tp;
}

void
delctype(register struct ctype *tp)
{
	if (!tp)
		return;
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
	while (stackp != stack)
		tp = mktype(tp, *--stackp);
	return tp;
}

struct ctype *
btype(struct ctype *tp, unsigned char tok)
{
	register unsigned char type;

	if (!tp)
		tp = newctype();

	type = tp->type;
	switch (tok) {
	case VOID: case BOOL: case STRUCT: case UNION: case ENUM:
		if (type)
			goto two_or_more;;
		type = tok;
		if (tp->c_signed || tp->c_unsigned)
			goto invalid_sign;
		break;
	case CHAR:
		if (type)
			goto two_or_more;
		type = CHAR;
		break;
	case SHORT:
		if (type && type != INT)
			goto two_or_more;
		type = SHORT;
		break;
	case INT:
		switch (type) {
		case 0:       type = INT;       break;
		case SHORT:   type = SHORT;     break;
		case LONG:    type = LONG;      break;
		default:      goto two_or_more;
		}
		break;
	case LONG:
		switch (type) {
		case 0:
		case INT:     type = LONG;      break;
		case LONG:    type = LLONG;     break;
		case DOUBLE:  type = LDOUBLE;   break;
		case LLONG:
		case LDOUBLE:  error("too much long");
		}
		break;
	case FLOAT:
		if (type)
			goto two_or_more;
		type = FLOAT;
		if (tp->c_signed || tp->c_unsigned)
			goto check_sign;
		break;
	case DOUBLE:
		if (type)
			goto two_or_more;
		if (!type)
			type = DOUBLE;
		else if (type == LONG)
			type = LDOUBLE;
		if (tp->c_signed || tp->c_unsigned)
			goto check_sign;
		break;
	case UNSIGNED:
		if (tp->c_unsigned)
			goto duplicated;
		if (tp->c_signed)
			goto both_sign;
		tp->c_unsigned = 1;
		goto check_sign;
	case SIGNED:
		if (tp->c_signed)
			goto duplicated;
		if (tp->c_unsigned)
			goto both_sign;
		tp->c_signed = 1;

check_sign:	switch (type) {
		case VOID: case BOOL: case STRUCT: case UNION: case ENUM:
			goto invalid_sign;
		}
		break;
	default:
		assert(0);
	}
	tp->type = type;
	return tp;

both_sign:
	error("both 'signed' and 'unsigned' in declaration specifiers");
duplicated:
	error("duplicated '%s'", yytext);
invalid_sign:
	error("invalid sign modifier");
two_or_more:
	error("two or more basic types");
}

struct ctype *
storage(register struct ctype *tp, unsigned char mod)
{
	extern unsigned char curctx;

	if (!tp)
		tp = newctype();
	switch (mod) {
	case TYPEDEF:
		if (tp->c_typedef)
			goto duplicated;
		if (tp->c_extern | tp->c_auto | tp->c_reg | tp->c_static)
			goto two_storage;
		if (tp->c_const || tp->c_volatile)
			goto bad_typedef;
		tp->c_typedef = 1;
		return tp;
	case EXTERN:
		if (tp->c_extern)
			goto duplicated;
		if (tp->c_typedef | tp->c_auto | tp->c_reg | tp->c_static)
			goto two_storage;
		tp->c_extern = 1;
		return tp;
	case STATIC:
		if (tp->c_static)
			goto duplicated;
		if (tp->c_typedef | tp->c_extern | tp->c_auto | tp->c_reg)
			goto two_storage;
		tp->c_static = 1;
		return tp;
	case AUTO:
		if (curctx == CTX_OUTER)
			goto bad_file_scope_storage;
		if (tp->c_typedef | tp->c_extern | tp->c_static | tp->c_reg)
			goto two_storage;
		if (tp->c_auto)
			goto duplicated;
		tp->c_static = 1;
		return tp;
	case REGISTER:
		if (curctx == CTX_OUTER)
			goto bad_file_scope_storage;
		if (tp->c_typedef | tp->c_extern | tp->c_auto | tp->c_static)
			goto two_storage;
		if (tp->c_reg)
			goto duplicated;
		tp->c_reg = 1;
		return tp;
	case CONST:
		if (options.repeat && tp->c_const)
			goto duplicated;
		if (tp->c_typedef)
			goto bad_typedef;
		tp->c_const = 1;
		return tp;
	case VOLATILE:
		if (options.repeat && tp->c_volatile)
			goto duplicated;
		if (tp->c_typedef)
			goto bad_typedef;
		tp->c_volatile = 1;
		return tp;
	}
bad_typedef:
	error("typedef specifies type qualifier");
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
