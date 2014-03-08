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
static unsigned char nr_tags = NS_TAG;
static unsigned char nested_tags;

static struct symbol *declarator(struct ctype *tp, unsigned char ns);

static struct symbol *
directdcl(register struct ctype *tp, unsigned char ns)
{
	register struct symbol *sym;

	if (accept('(')) {
		sym = declarator(tp, ns);
		expect(')');
	} else if (yytoken == IDEN) {
		sym = lookup(yytext, ns);
		if (!sym->ctype.defined)
			sym->ctx = curctx;
		else if (sym->ctx == curctx)
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
			return sym;
		}
	}
}

static unsigned char
newtag(unsigned char type)
{
	if (type == ENUM)
		return 0;
	if (nr_tags == NS_TAG + NR_MAXSTRUCTS)
		error("too much structs/unions defined");
	return ++nr_tags;
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
			if (aux->type != tp->type)
				goto bad_type;
			*tp = *aux;
		} else {
			tp->tag = sym->name;
			tp->ns = newtag(tp->type);
			sym->ctype = *tp;
		}
		next();
	} else {
		tp->ns = newtag(tp->type);
	}

	return sym;

bad_type:
	error("'%s' defined as wrong kind of tag", yytext);
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
		decl(tp->ns);
	--nested_tags;

	if (sym)
		sym->ctype.forward = 0;
	tp->forward = 0;
}

static void
enumdcl(struct ctype *base)
{
	static int val;

	aggregate(base);
	if (!accept('{'))
		return;
	val = 0;

	do {
		register struct symbol *sym;
		register struct ctype *tp;

		if (yytoken != IDEN)
			break;
		sym = lookup(yytext, NS_IDEN);
		tp = &sym->ctype;
		if (tp->defined && sym->ctx == curctx)
			error("'%s' redefined", yytext);
		next();
		if (accept('=')) {
			expect(CONSTANT);
			val = yyval->i;
		}
		ctype(tp, INT);
		tp->base = base;
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
			tp = ctype(tp, ENUM);
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

static struct symbol *
declarator(struct ctype *tp, unsigned char ns)
{
	unsigned char qlf[NR_DECLARATORS];
	register unsigned char *bp;
	register unsigned char n = 0;
	struct symbol *sym;

	if (yytoken == '*') {
		for (bp = qlf; n < NR_DECLARATORS ; ++n) {
			switch (yytoken) {
			case '*':
				yytoken = PTR;
			case CONST: case VOLATILE: case RESTRICT:
				*bp++ = yytoken;
				next();
				continue;
			default:
				goto direct;
			}
		}
		error("Too much type declarators");
	}

direct:	sym = directdcl(tp, ns);

	for (bp = qlf; n--; pushtype(*bp++))
		/* nothing */;
	return sym;
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
listdcl(struct ctype *base,
        struct storage *store, struct qualifier *qlf, unsigned char ns)
{
	struct compound c;
	char *err;

	c.tree = NULL;

	do {
		struct node *np, *aux;
		register struct ctype *tp;
		register struct symbol *sym;

		sym = declarator(base, ns);
		sym->store = *store;
		sym->qlf = *qlf;
		sym->ctype = *decl_type(base);
		tp = &sym->ctype;
		aux = NULL;

		switch (tp->type) {
		case FTN:
			if (ns != NS_IDEN)
				goto bad_type;
			if (yytoken == '{') {
				if (curctx != CTX_OUTER)
					goto local_fun;
				aux = function(sym);
				addstmt(&c, node(ODEF, nodesym(sym), aux));
				return c.tree;
			}
			goto add_stmt;
		case INT: case BOOL:
			if (ns != NS_IDEN && accept(':')) {
				expect(CONSTANT);
				tp = ctype(NULL, BITFLD);
				tp->len = yyval->i;
				goto add_stmt;
			}
			goto add_init;
		case STRUCT: case UNION:
			if (tp->forward)
				goto incomplete;
		default:
		add_init:
			if (ns == NS_IDEN) {
				if (accept('='))
					aux = initializer(tp);
			}
		add_stmt:
			addstmt(&c, node(ODEF, nodesym(sym), aux));
		}
	} while (accept(','));

	expect(';');
	return c.tree;

bad_type:
	err = "incorrect type for field";
	goto error;
local_fun:
	err = "cannot use local functions";
	goto error;
incomplete:
        err = "declaration of variable with incomplete type";
error: error(err);
}

struct node *
decl(unsigned char ns)
{
	struct ctype base;
	struct storage store;
	struct qualifier qlf;

repeat: initctype(&base);
	initstore(&store);
	initqlf(&qlf);

	if (!specifier(&base, &store, &qlf))
		return NULL;

	if (store.defined && ns != NS_IDEN)
		error("storage specifier in a struct/union field declaration");

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
			if (ns == NS_IDEN)
				break;
		default:
			warn(options.useless,
			     "useless type name in empty declaration");
		}
		if (yytoken == EOFTOK)
			return NULL;
		goto repeat;
	}
	return listdcl(&base, &store, &qlf, ns);
}

void
type_name()
{

}
