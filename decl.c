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
static void declarator(struct ctype *tp);

static void
newiden(struct ctype *tp)
{
	register unsigned char yyns, ns;

	ns = tp->c_typedef ? NS_TYPEDEF : NS_IDEN;
	yyns = yyval.sym->ns;

	if (yyns == NS_ANY) {     /* First appearence of the symbol */
		yyval.sym->ns = ns;
		cursym = yyval.sym;
		return;
	} else if (ns == yyns) {  /* Duplicated symbol */
		if (yyval.sym->ctx == curctx)
			error("redeclaration of '%s'", yytext);
	}
	cursym = lookup(yytext, ns);
}

static void
dirdcl(register struct ctype *tp)
{
	if (accept('(')) {
		declarator(tp);
		expect(')');
	} else if (yytoken == IDEN) {
		newiden(tp);
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
	unsigned char sign;
	register struct ctype *tp = NULL;

	for (sign = 0; ; next()) {
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
			if (!tp)
				tp = newctype();
			switch (tp->type) {
			case FLOAT: case DOUBLE: case LDOUBLE: case BOOL:
				goto invalid_sign;
			}
			if ((sign = yytoken) == UNSIGNED)
				tp->c_unsigned = 1;
			break;
		case FLOAT: case DOUBLE: case BOOL:
			if (sign)
				goto invalid_sign;
		case VOID: case CHAR: case SHORT: case INT: case LONG:
			tp = btype(tp, yytoken);
			break;
		case STRUCT:    /* TODO */
		case UNION:	/* TODO */
		case ENUM:	/* TODO */
		case IDEN:
			if (!tp || !tp->type) {
				struct symbol *sym;
				unsigned char tok = ahead();

				sym = (yyval.sym->ns == NS_TYPEDEF) ?
					yyval.sym : find(yytext, NS_TYPEDEF);
				if (sym && tok != ';' && tok != ',') {
					if (!tp)
						tp = newctype();
					tp->type = TYPEDEF;
					(tp->base = sym->ctype)->refcnt++;
					break;
				}
			}
			/* it is not a type name */
		default:
			if (!tp || tp->type)
				return tp;
			warning_error(options.implicit,
			              "type defaults to 'int' in declaration");
			tp->type = INT;
			return tp;
		}
	}
invalid_sign:
	error("invalid sign modifier");
}

static void
declarator(struct ctype *tp)
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

	dirdcl(tp);

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

		declarator(base);
		tp = decl_type(base);
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
		delctype(tp);
	} else {
		np = listdcl(tp);
	}

end:	return np;
}

void
type_name()
{

}
