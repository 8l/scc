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
		else                        /* don't create new symbol */
			return lookup(yytext, -ns);
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

static void
new_struct(register struct ctype *tp)
{
	struct symbol *sym = NULL;

	if (yytoken == IDEN) {
		sym = namespace(NS_STRUCT, -1);
		sym->ctype = tp;
		next();
	}
	if (nr_structs == NR_MAXSTRUCTS)
		error("too much structs/unions/enum defined");
	if (structp == &structbuf[NR_STRUCT_LEVEL])
		error("too much nested structs/unions");
	tp->ns = *structp++ = nr_structs;
	tp->forward = 1;
	tp->sym = sym;
}

static struct ctype *spec(void);

static struct ctype *
field_dcl(unsigned char ns)
{
	register struct ctype *tp, *base;

	if (!(base = spec())) {
		base = newctype();
		base->type = INT;
		warn(options.implicit,
		     "data definition has no type or storage class");
	}
	if (HAS_STORAGE(base))
		error("storage specifier in a struct/union field declaration");

	do {
		declarator(base, ns);
		tp = decl_type(base);
		if (accept(':')) {
			expect(CONSTANT);
			switch (tp->type) {
			case INT: case BOOL:
				tp = btype(NULL, BITFLD);
				tp->len = yyval.sym->val;
				break;
			default:
				error("bit-field '%s' has invalid type",
				      cursym->name);
			}
		}
		cursym->ctype = tp;
	} while (accept(','));

	delctype(base);
	expect(';');
	return tp;
}

static struct ctype *
struct_dcl(register struct ctype *tp)
{
	new_struct(tp);

	if (!accept('{'))
		return tp;
	if (!tp->forward)
		error("struct/union already defined");

	do
		field_dcl(tp->ns);
	while (!accept('}'));

	tp->forward = 0;
	return tp;
}

static struct ctype *
enum_dcl(struct ctype *base)
{
	short val = 0;

	new_struct(base);
	if (!accept('{'))
		return base;

	do {
		register struct symbol *sym;
		register struct ctype *tp = btype(NULL, INT);

		if (yytoken == '}')
			break;

		expect(IDEN);
		sym = namespace(NS_IDEN, 1);
		sym->ctype = tp;
		if (accept('=')) {
			expect(CONSTANT);
			val = yyval.sym->val;
		}
		sym->val = val++;
	} while (accept(','));

	expect('}');

	return base;
}

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
		case INT:      case LONG:
			tp = btype(tp, yytoken);
			break;
		case ENUM:
			tp = btype(tp, yytoken);
			next();
			return enum_dcl(tp);
		case STRUCT:   case UNION:
			tp = btype(tp, yytoken);
			next();
			return struct_dcl(tp);
		case IDEN:
			if (!tp || !tp->type) {
				struct symbol *sym;
				unsigned char tok = ahead();

				sym = namespace(NS_TYPEDEF, 0);
				if (sym && tok != ';' && tok != ',') {
					if (!tp)
						tp = newctype();
					tp->type = TYPEDEF;
					tp->base = sym->ctype;
					break;
				}
			}
			/* it is not a type name */
		default:
			if (!tp)
				return NULL;
			if (!tp->type) {
				warn(options.implicit,
			             "type defaults to 'int' in declaration");
				tp->type = INT;
			}
			if (!tp->c_signed && !tp->c_unsigned) {
				switch (tp->type) {
				case CHAR:
					if (!options.charsign) {
				case BOOL:	tp->c_unsigned = 1;
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

	for (lim = bp, bp = qlf; bp < lim; ++bp)
		pushtype(*bp);
}

static struct node *
initializer(register struct ctype *tp)
{
	if (accept('{')) {
		struct compound c;

		c.tree = NULL;
		addstmt(&c, initializer(tp));
		while (accept(',')) {
			if (accept('}'))
				return c.tree;
			addstmt(&c, initializer(tp));
		}
		expect('}');
		return c.tree;
	} else {
		return expr();
	}
}

static struct node *
listdcl(struct ctype *base)
{
	struct compound c;

	c.tree = NULL;

	do {
		struct node *np;
		register struct ctype *tp;

		declarator(base, base->c_typedef ? NS_TYPEDEF : NS_IDEN);
		tp = cursym->ctype = decl_type(base);
		if ((tp->type == STRUCT || tp->type == UNION) && tp->forward)
			error("declaration of variable with incomplete type");

		np = nodesym(cursym);
		if (tp->type == FTN && yytoken == '{') {
			np  = node(ODEF, np, function(cursym));
			return addstmt(&c, np);
		}
		np = node(ODEF, np, accept('=') ? initializer(tp) : NULL);
		addstmt(&c, np);
	} while (accept(','));

	delctype(base);
	expect(';');
	return c.tree;
}

struct node *
decl(void)
{
	register struct ctype *base;

repeat: if (!(base = spec())) {
		if (curctx != CTX_OUTER || yytoken != IDEN)
			return NULL;
		base = newctype();
		base->type = INT;
		warn(options.implicit,
		     "data definition has no type or storage class");
	} else if (accept(';')) {
		register unsigned char type = base->type;

		if (type == STRUCT || type == UNION || type == ENUM) {
			if (HAS_STORAGE(base) || HAS_QUALIF(base)) {
				warn(options.useless,
				     "useless storage class specifier in empty declaration");
			}
			if (!base->sym && type != ENUM) {
				warn(options.useless,
				     "unnamed struct/union that defines no instances");
			}
		} else {
			warn(options.useless,
			     "useless type name in empty declaration");
		}
		delctype(base);
		goto repeat;
	}
	return listdcl(base);
}

void
type_name()
{

}
