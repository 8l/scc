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
static unsigned char nr_structs = NS_STRUCT;
static unsigned char nested_struct;

static void declarator(struct ctype *tp, unsigned char ns);

static void
directdcl(register struct ctype *tp, unsigned char ns)
{
	if (accept('(')) {
		declarator(tp, ns);
		expect(')');
	} else if (yytoken == IDEN) {
		cursym = lookup(yytext, ns);
		if (!cursym->ctype)
			cursym->ctx = curctx;
		else if (cursym->ctx == curctx)
			error("redeclaration of '%s'", yytext);
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
		} else if (accept('[')) {
			unsigned len;

			if (accept(']')) {
				len = 0;
			} else {
				expect(CONSTANT);
				len = yyval->i;
				expect(']');
			}
			pushtype(len);
			pushtype(ARY);
		} else {
			return;
		}
	}
}

static void
aggregate(register struct ctype *tp)
{
	struct symbol *sym = NULL;

	if (yytoken == IDEN) {
		sym = lookup(yytext, NS_STRUCT);
		sym->ctype = tp;
		next();
	}
	if (nr_structs == NS_STRUCT + NR_MAXSTRUCTS)
		error("too much structs/unions/enum defined");
	tp->ns = ++nr_structs;
	tp->forward = 1;
	tp->sym = sym;
}

static struct ctype *specifier(void);

static struct ctype *
fielddcl(unsigned char ns)
{
	register struct ctype *tp, *base;

	if (!(base = specifier())) {
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
				tp = ctype(NULL, BITFLD);
				tp->len = yyval->i;
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
structdcl(register struct ctype *tp)
{
	aggregate(tp);
	if (nested_struct == NR_STRUCT_LEVEL)
		error("too much nested structs/unions");

	++nested_struct;
	if (!accept('{'))
		return tp;
	if (!tp->forward)
		error("struct/union already defined");

	do
		fielddcl(tp->ns);
	while (!accept('}'));
	--nested_struct;

	tp->forward = 0;
	return tp;
}

static struct ctype *
enumdcl(struct ctype *base)
{
	short val = 0;

	aggregate(base);
	if (!accept('{'))
		return base;

	do {
		register struct symbol *sym;
		register struct ctype *tp = ctype(NULL, INT);

		if (yytoken != IDEN)
			break;
		sym = lookup(yytext, NS_IDEN);
		sym->ctype = tp;
		next();
		if (accept('=')) {
			expect(CONSTANT);
			val = yyval->i;
		}
		sym->i = val++;
	} while (accept(','));

	expect('}');

	return base;
}

struct ctype *
specifier(void)
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
			tp = ctype(tp, yytoken);
			break;
		case ENUM:
			tp = ctype(tp, yytoken);
			next();
			return enumdcl(tp);
		case STRUCT:   case UNION:
			tp = ctype(tp, yytoken);
			next();
			return structdcl(tp);
		case IDEN:
			/* TODO: remove NS_TYPEDEF */
			if (tp && tp->c_typedef && !tp->type)
				goto check_type;
			if (!tp || !tp->type) {
				struct symbol *sym = lookup(yytext, NS_TYPEDEF);

				if (sym->ctype) {
					tp = ctype(tp, TYPEDEF);
					tp->base = sym->ctype;
					break;
				}
			}
			/* it is not a type name */
		default:
		check_type:
			if (!tp) {
				if (curctx != CTX_OUTER || yytoken != IDEN)
					return NULL;
				tp = ctype(tp, INT);
				warn(options.implicit,
					"data definition has no type or storage class");
			} else if (!tp->type) {
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

	directdcl(tp, ns);

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

	expect(';');
	return c.tree;
}

struct node *
decl(void)
{
	register struct ctype *base;
	struct node *np;

repeat: if (!(base = specifier()))
		return NULL;

	if (accept(';')) {
		register unsigned char type = base->type;

		switch (type) {
		case STRUCT: case UNION: case ENUM:
			if (HAS_STORAGE(base) || HAS_QUALIF(base)) {
				warn(options.useless,
				     "useless storage class specifier in empty declaration");
			}
			if (!base->sym && type != ENUM) {
				warn(options.useless,
				     "unnamed struct/union that defines no instances");
			}
		default:
			warn(options.useless,
			     "useless type name in empty declaration");
		}
		delctype(base);
		goto repeat;
	}
	np = listdcl(base);
	delctype(base);
	return np;
}

void
type_name()
{

}
