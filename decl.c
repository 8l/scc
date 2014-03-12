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
			if (!sym->ctype.type)
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

static void structdcl(register struct ctype *tp);
static void enumdcl(struct ctype *base);

static void
specifier(register struct ctype *tp, char *store, char *qlf)
{
	unsigned char tok;

	for (;; next()) {
		switch (yytoken) {
		case TQUALIFIER:
			if (*qlf && !options.repeat)
				error("duplicated '%s'", yytext);
			if (yyval->c == RESTRICT)
				error("invalid use of restrict");
			*qlf |= yyval->c;
			break;
		case STORAGE:
			if (*store)
				error("two or more storage specifier");
			/* TODO: check bad storage in file-scope */
			*store |= yyval->c;
			break;
		case TYPE:
			tp = ctype(tp, tok = yyval->c);
			switch (tok) {
			case ENUM: case STRUCT: case UNION:
				next();
				if (tok == ENUM)
					enumdcl(tp);
				else
					structdcl(tp);
				return;
			case TYPENAME:
				tp->base = &yyval->ctype;
				break;
			}
			break;
		default:
			goto check_type;
		}
	}

check_type:
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
	} else if (!tp->type) {
		tp->type = INT;
	}
	return;
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
			if (yytoken == '*')
				*bp++ = PTR;
			else if (yytoken == TQUALIFIER)
				*bp++ = yyval->c;
			else
				goto direct;
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
        char store, char qlf,
	unsigned char ns, unsigned char isfun)
{
	struct compound c;
	char *err;

	c.tree = NULL;

	if (yytoken == ';')
		return NULL;

	do {
		struct node *np, *aux;
		register struct ctype *tp;
		register struct symbol *sym;

		sym = declarator(base, ns, isfun);
		sym->store = store;
		sym->qlf = qlf;
		sym->ctype = *decl_type(base);
		if (sym->store) {
			sym->tok = TYPE;
			sym->c = TYPENAME;
		}
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
		if (aux->type) {
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
	while (!accept('}')) {
		struct ctype base;
		struct node *np;
		char store = 0, qlf = 0;

		initctype(&base);
		specifier(&base, &store, &qlf);

		if (store)
			error("storage specifier in a struct/union field declaration");

		listdcl(&base, store, qlf, tp->ns, 0);
		expect(';');
	}
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
		if (tp->type && sym->ctx == curctx)
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

struct node *
decl(unsigned char ns)
{
	struct ctype base;
	struct node *np;
	char store = 0, qlf = 0;

	initctype(&base);
	specifier(&base, &store, &qlf);

	if (store && ns != NS_IDEN)
		error("storage specifier in a struct/union field declaration");

	np = listdcl(&base, store, qlf, ns, 0);
	expect(';');
	return np;
}

void
type_name(struct ctype *tp)
{
	char store = 0, qlf = 0;

	initctype(tp);
	specifier(tp, &store, &qlf);
	declarator(tp, NS_TYPE, 0);
	return;
}

struct node *
extdecl(void)
{
	struct ctype base;
	struct node *np;
	char store = 0, qlf = 0;

	initctype(&base);

	switch (yytoken) {
	case IDEN:
		warn(options.implicit,
		     "type defaults to 'int' in declaration");
		base.type = INT;
		break;
	case TYPE: case STORAGE: case TQUALIFIER:
		specifier(&base, &store, &qlf);
		break;
	default:
		error("declaration expected");
	}

	np = listdcl(&base, store, qlf, 0, 0);
	expect(';');
}