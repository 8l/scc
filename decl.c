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

static struct symbol *declarator(struct ctype *tp,
                                 unsigned char ns, unsigned char isfun);

static struct symbol *
directdcl(register struct ctype *tp, unsigned char ns, unsigned char isfun)
{
	register struct symbol *sym;
	register char *err;

	if (accept('(')) {
		sym = declarator(tp, ns, isfun);
		expect(')');
	} else if (ns != NS_TYPE) {
		if (yytoken == IDEN) {
			sym = lookup(yytext, ns);
			if (!sym->ctype.defined)
				sym->ctx = curctx;
			else if (sym->ctx == curctx)
				goto redeclared;
			next();
		} else if (!isfun) {
			goto expected;
		}
	}

	for (;;) {
		if (accept('(')) {
			pushtype(FTN);
			if (yytoken != ')')
				;   /* TODO: prototyped function */;
			expect(')');
		} else if (accept('[')) {
			unsigned len = '0';

			if (yytoken != ']') {
				expect(CONSTANT);
				len = yyval->i;
			}
			expect(']');
			pushtype(len);
			pushtype(ARY);
		} else {
			return sym;		
		}
	}

redeclared:
	err = "redeclaration of '%s'";
	goto error;
expected:
	err = "expected '(' or identifier before of '%s'";
error:	error(err, yytext);
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
	unsigned char tok;

	for (;; next()) {
		switch (yytoken) {
		case TQUALIFIER:
			qlf = qualifier(qlf, yyval->c);
			break;
		case STORAGE:
			store = storage(store, yyval->c);
			break;
		case TYPE:
			tp = ctype(tp, tok = yyval->c);
			switch (tok) {
			case ENUM: case STRUCT: case UNION:
				next();
				(tok == ENUM) ? enumdcl(tp) : structdcl(tp);
				return true;
			}
			break;
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
declarator(struct ctype *tp, unsigned char ns, unsigned char isfun)
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

direct:	sym = directdcl(tp, ns, isfun);

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
        struct storage *store,
	struct qualifier *qlf,
	unsigned char ns, unsigned char isfun)
{
	struct compound c;
	char *err;

	c.tree = NULL;

	do {
		struct node *np, *aux;
		register struct ctype *tp;
		register struct symbol *sym;

		sym = declarator(base, ns, isfun);
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

	initctype(&base);
	initstore(&store);
	initqlf(&qlf);

	if (!specifier(&base, &store, &qlf))
		return NULL;

	if (store.defined && ns != NS_IDEN)
		error("storage specifier in a struct/union field declaration");

	switch (base.type) {
	case STRUCT: case UNION: case ENUM:
		if (yytoken == ';')
			return NULL;
	default:
		return listdcl(&base, &store, &qlf, ns, 0);
	}
}

bool
type_name(struct ctype *tp)
{
	struct storage store;
	struct qualifier qlf;

	initctype(tp);
	initstore(&store);
	initqlf(&qlf);

	if (!specifier(tp, &store, &qlf))
		return false;

	declarator(tp, NS_TYPE, 0);
	return true;
}

