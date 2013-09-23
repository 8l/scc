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
static unsigned char nr_structs = NS_ANY + 1;
static unsigned char structbuf[NR_STRUCT_LEVEL], *structp = &structbuf[0];

static void declarator(struct ctype *tp, unsigned char ns);

static struct symbol *
namespace(register unsigned char ns, signed char alloc)
{
	register struct symbol *sym = yyval.sym;
	unsigned char yyns = sym->ns;

	if (!alloc) {
		if (yyns == NS_ANY)
			return NULL;
		else if (yyns == ns)
			return sym;
		else
			return find(yytext, ns);
	} else {
		if (yyns == NS_ANY) {
			sym->ns = ns;
			return sym;
		} else if (yyns == ns && sym->ctx == curctx) {
			if (alloc < 0)
				return sym;
			error("redeclaration of '%s'", yytext);
		}
		return lookup(yytext, ns);
	}
}

static void
dirdcl(register struct ctype *tp, unsigned char ns)
{
	if (accept('(')) {
		declarator(tp, ns);
		expect(')');
	} else if (yytoken == IDEN) {
		cursym = namespace(ns, 1);
		next();
	} else {
		error("expected '(' or identifier before of '%s'", yytext);
	}

	for (;;) {
		if (accept('(')) {
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


static struct ctype *struct_spec(register struct ctype *);

struct ctype *
spec(void)
{
	register struct ctype *tp = NULL;

	for (;; next()) {
		switch (yytoken) {
		case TYPEDEF:  case EXTERN: case STATIC: case AUTO:
		case REGISTER: case CONST:  case VOLATILE:
			tp = storage(tp, yytoken);
			break;
		case UNSIGNED: case SIGNED:
		case COMPLEX:  case IMAGINARY:
		case FLOAT:    case DOUBLE: case BOOL:
		case VOID:     case CHAR:   case SHORT:
		case INT:      case ENUM:   case LONG:
			tp = btype(tp, yytoken);
			break;
			/* TODO: ENUM */
		case STRUCT:   case UNION:
			tp = btype(tp, yytoken);
			next();
			return struct_spec(tp);
		case IDEN:
			if (!tp || !tp->type) {
				struct symbol *sym;
				unsigned char tok = ahead();

				sym = namespace(NS_TYPEDEF, 0);
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
			if (!tp)
				return NULL;
			if (!tp->type) {
				warning_error(options.implicit,
			                      "type defaults to 'int' in declaration");
				tp->type = INT;
			}
			if (!tp->c_signed && !tp->c_unsigned) {
				switch (tp->type) {
				case CHAR:
					if (!options.charsign) {
						tp->c_unsigned = 1;
						break;
					}
				case INT: case SHORT: case LONG: case LLONG:
					tp->c_signed = 1;
				}
			}
			return tp;
		}
	}
}

static struct ctype *
struct_dcl(unsigned char ns)
{
	register struct ctype *tp, *base;

	if (!(base = spec())) {
		base = newctype();
		base->type = INT;
		warning_error(options.implicit,
		              "data definition has no type or storage class");
	}
	if (base->c_typedef  || base->c_static || base->c_auto ||
	    base->c_register || base->c_extern) {
		error("storage specifier in a struct/union field declaration");
	}

	do {                      /* TODO: detect unnamed structs */
		declarator(base, ns);
		tp = decl_type(base);
		(cursym->ctype = tp)->refcnt++;
	} while (accept(','));

	expect(';');
	return tp;
}

static struct ctype *
struct_spec(register struct ctype *tp)
{
	if (yytoken == IDEN) {
		(namespace(NS_STRUCT, -1)->ctype = tp)->refcnt++;
		next();
	}

	if (nr_structs == NR_MAXSTRUCTS)
		error("too much structs/unions defined");
	if (structp == &structbuf[NR_STRUCT_LEVEL])
		error("too much nested structs/unions");
	tp->c_struct = *structp++ = nr_structs;

	if (!accept('{'))
		return tp;

	if (!tp->forward)
		error("struct/union already defined");

	do
		struct_dcl(tp->c_struct);
	while (!accept('}'));
	tp->forward = 0;

	return tp;
}

static void
declarator(struct ctype *tp, unsigned char ns)
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

	dirdcl(tp, ns);

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

		declarator(base, base->c_typedef ? NS_TYPEDEF : NS_IDEN);
		tp = decl_type(base);
		(cursym->ctype = tp)->refcnt++;
		if ((tp->type == STRUCT || tp->type == UNION) && tp->forward)
			error("declaration of variable with incomplete type");

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

repeat: if (!(tp = spec())) {
		if (curctx != CTX_OUTER || yytoken != IDEN)
			return NULL;
		tp = newctype();
		tp->type = INT;
		warning_error(options.implicit,
		              "data definition has no type or storage class");
	} else if (accept(';')) {
		register unsigned char type = tp->type;

		if (type == STRUCT || type == UNION) {
			if (tp->c_extern || tp->c_static || tp->c_auto ||
			    tp->c_register || tp->c_const || tp->c_volatile) {
				warning_error(options.useless,
				              "useless storage class specifier in empty declaration");
			}
		} else {
			warning_error(options.useless,
			              "useless type name in empty declaration");
		}
		delctype(tp);
		goto repeat;
	}
	return listdcl(tp);
}

void
type_name()
{

}
