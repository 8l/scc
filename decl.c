#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "syntax.h"
#include "symbol.h"

char parser_out_home;
/*
 * Number of nested declarations:
 * Number of nested struct declarations
 * +1 for the function declaration
 * +1 for the field declaration
 */
static struct symbol *symstack[NR_STRUCT_LEVEL + 1 + 1], **symstackp = symstack;
static struct symbol *cursym;
static unsigned char nr_tags = NS_TAG;
static unsigned char nested_tags;

static void declarator(struct ctype *tp, unsigned char ns);

static void
directdcl(register struct ctype *tp, unsigned char ns)
{
	if (accept('(')) {
		declarator(tp, ns);
		expect(')');
	} else if (yytoken == IDEN) {
		/* we can't overflow due to the check in structdcl */
		*symstackp++ = cursym = lookup(yytext, ns);
		if (!cursym->ctype.defined)
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

static struct symbol *
aggregate(register struct ctype *tp)
{
	struct symbol *sym = NULL;

	tp->forward = 1;
	if (yytoken == IDEN) {
		register struct ctype *aux;

		sym = lookup(yytext, NS_TAG);
		aux = &sym->ctype;
		if (aux->defined) {
			if (aux->type != tp->type) {
				error("'%s' defined as wrong kind of tag",
				      yytext);
			}
			*tp = *aux;
		} else {
			tp->tag = sym->name;
			tp->ns = ++nr_tags;
			sym->ctype = *tp;
		}
		next();                     /* This way of handling nr_tag */
	} else {                            /* is incorrect once is incorrect*/
		tp->ns = ++nr_tags;         /* it will be incorrect forever*/
	}

	if (nr_tags == NS_TAG + NR_MAXSTRUCTS)
		error("too much structs/unions/enum defined");

	return sym;
}

static bool
specifier(register struct ctype *, struct storage *, struct qualifier *);

static void
fielddcl(unsigned char ns)
{
	struct ctype base;
	struct storage store;
	struct qualifier qlf;
	register struct ctype *tp;

	initctype(&base);
	initstore(&store);
	initqlf(&qlf);
	specifier(&base, &store, &qlf);

	if (store.defined)
		error("storage specifier in a struct/union field declaration");

	do {
		declarator(&base, ns);
		tp = decl_type(&base);
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
		cursym->ctype = *tp;
		cursym->qlf = qlf;
	} while (accept(','));

	expect(';');
}

static void
structdcl(register struct ctype *tp)
{
	struct symbol *sym;

	sym = aggregate(tp);

	if (!accept('{'))
		return;

	if (sym && !sym->ctype.forward)
		error("struct/union already defined");

	if (nested_tags == NR_STRUCT_LEVEL)
		error("too much nested structs/unions");

	++nested_tags;
	while (!accept('}'))
		fielddcl(tp->ns);
	--nested_tags;

	if (sym)
		sym->ctype.forward = 0;
	tp->forward = 0;
}

static void
enumdcl(struct ctype *base)
{
	short val = 0;

	aggregate(base);
	if (!accept('{'))
		return;

	do {
		register struct symbol *sym;
		register struct ctype *tp = ctype(NULL, INT);

		if (yytoken != IDEN)
			break;
		sym = lookup(yytext, NS_IDEN);
		sym->ctype = *tp;
		next();
		if (accept('=')) {
			expect(CONSTANT);
			val = yyval->i;
		}
		sym->i = val++;
	} while (accept(','));

	expect('}');
}

bool
specifier(register struct ctype *tp,
          struct storage *store, struct qualifier *qlf)
{
	for (;; next()) {
		switch (yytoken) {
		case CONST:  case VOLATILE:
			qlf = qualifier(qlf, yytoken);
			break;
		case TYPEDEF:  case EXTERN: case STATIC:
		case AUTO:     case REGISTER:
			store = storage(store, yytoken);
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
			enumdcl(tp);
			return true;
		case STRUCT:   case UNION:
			tp = ctype(tp, yytoken);
			next();
			structdcl(tp);
			return true;
		case IDEN:
			if (!tp->defined) {
				register struct symbol *sym;

				sym = lookup(yytext, NS_IDEN);
				if (sym->ctype.defined &&
				    sym->store.c_typedef) {
					tp = ctype(tp, TYPEDEF);
					tp->base = &sym->ctype;
					break;
				}
			}
			/* it is not a type name */
		default:
			goto check_type;
		}
	}

check_type:
	if (!tp->defined) {
		if (!store->defined &&
		    !qlf->defined &&
		    curctx != CTX_OUTER &&
		    nested_tags == 0) {
			return false;
		}
		warn(options.implicit,
		     "type defaults to 'int' in declaration");
	}
	if (nested_tags > 0 && (qlf->defined || store->defined))
		error("type qualifer or store specifier in field declaration");

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
	return true;
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
	if (!accept('='))
		return NULL;
	next();
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
listdcl(struct ctype *base, struct storage *store, struct qualifier *qlf)
{
	struct compound c;
	char fun;

	c.tree = NULL;

	do {
		struct node *np, *aux;
		register struct ctype *tp;

		declarator(base, NS_IDEN);
		cursym->store = *store;
		cursym->qlf = *qlf;
		cursym->ctype = *decl_type(base);
		tp = &cursym->ctype;
		if ((tp->type == STRUCT || tp->type == UNION) && tp->forward)
			error("declaration of variable with incomplete type");

		np = nodesym(cursym);
		fun = tp->type == FTN && yytoken == '{';
		aux = fun ? function(cursym) : initializer(tp);
		addstmt(&c, node(ODEF, np, aux));
		cursym = *--symstackp;
	} while (!fun && accept(','));

	if (!fun)
		expect(';');
	return c.tree;
}

struct node *
decl(void)
{
	struct ctype base;
	struct storage store;
	struct qualifier qlf;

repeat: initctype(&base);
	initstore(&store);
	initqlf(&qlf);

	if (!specifier(&base, &store, &qlf))
		return NULL;

	if (accept(';')) {
		register unsigned char type = base.type;

		switch (type) {
		case STRUCT: case UNION:
			if (!base.tag) {
				warn(options.useless,
				     "unnamed struct/union that defines no instances");
			}
		case ENUM:
			if (store.defined) {
				warn(options.useless,
				     "useless storage class specifier in empty declaration");
			}
			if (qlf.defined) {
				warn(options.useless,
				     "useless type qualifier in empty declaration");
			}
			break;
		default:
			warn(options.useless,
			     "useless type name in empty declaration");
		}
		if (yytoken == EOFTOK)
			return NULL;
		goto repeat;
	}
	return listdcl(&base, &store, &qlf);
}

void
type_name()
{

}
