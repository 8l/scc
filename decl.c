#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "syntax.h"
#include "symbol.h"

char parser_out_home;

static struct symbol *cursym;
static void declarator(void);

static void
newiden(void)
{
	switch (yyval.sym->ns) {
	case NS_ANY:        /* First aparrence of the symbol */
		yyval.sym->ns = NS_IDEN;
		cursym = yyval.sym;
		break;
	case NS_IDEN:
		if (yyval.sym->ctx == curctx)
			error("redeclaration of '%s'", yytext);
	default:
		cursym = lookup(yytext, NS_IDEN);
	}
}

static void
dirdcl(void)
{
	if (accept('(')) {
		declarator();
		expect(')');
	} else if (yytoken == IDEN) {
		newiden();
		next();
	} else {
		error("expected '(' or identifier before of '%s'", yytext);
	}

	for (;;) {
		if (accept('(')) {
			next();
			pushtype(FTN);
			if (accept(')'))
				; /* TODO: k&r function */
			else
				/* TODO: prototyped function */;
			continue;
		} else if (accept('[')) {
			unsigned len;

			if (accept(']')) {
				len = 0;
			} else {
				expect(CONSTANT);
				len = yyval.sym->val;
				expect(']');
			}
			pushtype(len);
			pushtype(ARY);
			continue;
		} else {
			return;
		}
	}
}

struct ctype *
spec(void)
{
	static unsigned char sign, type;
	register struct ctype *tp = NULL;

	for (type = sign = 0; ; next()) {
		switch (yytoken) {
		case TYPEDEF:  case EXTERN: case STATIC: case AUTO:
		case REGISTER: case CONST:  case VOLATILE:
			tp = storage(tp, yytoken);
			break;
		case UNSIGNED: case SIGNED:
			if (sign == yytoken)
				error("duplicated '%s'", yytext);
			if (sign)
				error("both 'signed' and 'unsigned' in declaration specifiers");
			switch (type) {
			case FLOAT: case DOUBLE: case LDOUBLE:
				goto float_sign;
			}
			if (!tp)
				tp = newctype();
			if ((type = sign = yytoken) == UNSIGNED)
				tp->c_unsigned = 1;
			break;
		case FLOAT: case DOUBLE:
			if (sign)
				goto float_sign;
		case VOID:   case CHAR:   case SHORT:
		case INT:    case LONG:   case BOOL:
			tp = btype(tp, yytoken);
			type = tp->type;
			break;
		case STRUCT:    /* TODO */
		case UNION:	/* TODO */
		case ENUM:	/* TODO */
		default:
			if (tp && !tp->type && sign)
				tp->type = INT;
			return tp;
		}
	}
float_sign:
	error("floating types cannot be signed or unsigned");
}

static void
declarator(void)
{
	unsigned char qlf[NR_DECLARATORS];
	register unsigned char *bp, *lim;

	lim = &qlf[NR_DECLARATORS];
	for (bp = qlf; yytoken == '*' && bp != lim; ) {
		next();
		*bp++ = PTR;
		while (bp != lim) {
			switch (yytoken) {
			case CONST: case VOLATILE: case RESTRICT:
				*bp++ = yytoken;
				next();
				break;
			default:
				goto continue_outer;
			}
		}
	continue_outer: ;
	}
	if (bp == lim)
		error("Too much type declarators");

	dirdcl();

	for (lim = bp - 1, bp = qlf; bp < lim; ++bp)
		pushtype(*bp);
}

static struct node *
initializer(register struct ctype *tp)
{
	register struct node *np;

	if (accept('{')) {
		np = nodecomp();
		addstmt(np, initializer(tp));
		while (accept(',')) {
			if (accept('}'))
				return np;
			addstmt(np, initializer(tp));
		}
		expect('}');
	} else {
		np = expr();
	}
	return np;
}

static struct node *
listdcl(struct ctype *base)
{
	struct node *lp = nodecomp();

	do {
		struct node *sp, *np;
		register struct ctype *tp;

		declarator();
		tp = decl_type(base);
		if (!tp->type) {
			warning_error(options.implicit,
				      "type defaults to 'int' in declaration of '%s'",
				      yytext);
		}
		(cursym->ctype = tp)->refcnt++;
		sp = nodesym(cursym);
		if (tp->type == FTN && yytoken == '{') {
			np  = node2(ODEF, sp, function(cursym));
			return addstmt(lp, np);
		}
		np = node2(ODEF, sp, accept('=') ? initializer(tp) : NULL);
		lp = addstmt(lp, np);
	} while (accept(','));
	expect(';');

	return lp;
}

struct node *
decl(void)
{
	register struct ctype *tp;
	register struct node *np = NULL;

	while (accept(';'))
		/* nothing */;
	if (!(tp = spec())) {
		if (curctx != CTX_OUTER)
			goto end;
		warning("data definition has no type or storage class");
	}
	if (accept(';')) {
		warning_error(options.useless,
			      "useless type name in empty declaration");
	} else {
		np = listdcl(tp);
	}

end:	delctype(tp);
	return np;
}

void
type_name()
{

}
