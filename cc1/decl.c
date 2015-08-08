
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

struct dcldata {
	unsigned char op;
	unsigned short nelem;
	unsigned char ndcl;
	void *data;
};

static struct dcldata *
queue(struct dcldata *dp, unsigned op, short nelem, void *data)
{
	unsigned n;

	if ((n = dp->ndcl) == NR_DECLARATORS)
		error("too much declarators");
	dp->op = op;
	dp->nelem = nelem;
	dp->data = data;
	++dp;
	dp->ndcl = n+1;
	return dp;
}

static struct dcldata *
arydcl(struct dcldata *dp)
{
	Node *np = NULL;
	TINT n;

	expect('[');
	if (yytoken != ']') {
		if ((np = iconstexpr()) == NULL)
			error("invalid storage size");
	}
	expect(']');

	n = (np == NULL) ? 0 : np->sym->u.i;
	freetree(np);

	return queue(dp, ARY, n, NULL);
}

static void
parameter(Symbol *sym, int sclass, Type *data)
{
	Type *tp = sym->type, *funtp = data;
	size_t n = funtp->n.elem;

	if (tp == voidtype) {
		if (n != 0)
			error("incorrect void parameter");
		funtp->n.elem = -1;
		return;
	}

	if (n == -1)
		error("'void' must be the only parameter");
	tp = sym->type;
	if (tp->op == FTN)
		error("incorrect function type for a function parameter");
	if (tp->op == ARY)
		tp = mktype(tp->type, PTR, 0, NULL);
	if (sym->flags & (ISSTATIC|ISEXTERN|ISAUTO))
		error("bad storage class in function parameter");
	if (!sclass)
		sym->flags |= ISAUTO;
	if (n++ == NR_FUNPARAM)
		error("too much parameters in function definition");
	funtp->pars = xrealloc(funtp->pars, n * sizeof(Type *));
	funtp->pars[n-1] = tp;
	funtp->n.elem = n;
}

static Symbol *dodcl(int rep,
                     void (*fun)(Symbol *, int, Type *),
                     uint8_t ns, Type *type);

static struct dcldata *
fundcl(struct dcldata *dp)
{
	Type type = {.n = {.elem = -1}, .pars = NULL};
	Symbol *syms[NR_FUNPARAM], **sp;
	size_t size;
	void *p;

	pushctx();
	expect('(');

	if (accept(')')) {
		dp = queue(dp, FTN, type.n.elem, type.pars);
	} else {
		type.n.elem = 0;
		sp = syms;
		do
			*sp++ = dodcl(0, parameter, NS_IDEN, &type);
		while (accept(','));

		expect(')');

		dp = queue(dp, FTN, type.n.elem, type.pars);
		if (type.n.elem != -1) {
			size = type.n.elem * sizeof(Symbol *);
			p = memcpy(xmalloc(size), syms, size);
			dp = queue(dp, PARS, 0, p);
		}
	}

	switch (yytoken) {
	default:
		/* This is not a function */
		popctx();
	case '{':
	case TYPEIDEN:
	case TYPE:
	case TQUALIFIER:
	case SCLASS:
		/* This can be a function (K&R included) */
		break;
	}
	return dp;
}

static struct dcldata *declarator0(struct dcldata *dp, unsigned ns);

static struct dcldata *
directdcl(struct dcldata *dp, unsigned ns)
{
	Symbol *sym;

	if (accept('(')) {
		dp = declarator0(dp, ns);
		expect(')');
	} else {
		if (yytoken == IDEN || yytoken == TYPEIDEN) {
			sym = yylval.sym;
			next();
		} else {
			sym = newsym(ns);
		}
		dp = queue(dp, IDEN, 0, sym);
	}

	for (;;) {
		switch (yytoken) {
		case '(':  dp = fundcl(dp); break;
		case '[':  dp = arydcl(dp); break;
		default:   return dp;
		}
	}
}

static struct dcldata*
declarator0(struct dcldata *dp, unsigned ns)
{
	unsigned  n;

	for (n = 0; accept('*'); ++n) {
		while (accept(TQUALIFIER))
			/* nothing */;
	}

	dp = directdcl(dp, ns);

	while (n--)
		dp = queue(dp, PTR, 0, NULL);

	return dp;
}

static Symbol *
declarator(Type *tp, unsigned ns, int sclass)
{
	struct dcldata data[NR_DECLARATORS+1];
	struct dcldata *bp;
	Symbol *osym, *sym, **pars = NULL;
	char *name;

	data[0].ndcl = 0;
	for (bp = declarator0(data, ns); bp-- > data; ) {
		switch (bp->op) {
		case IDEN:
			sym = bp->data;
			break;
		case PARS:
			pars = bp->data;
			break;
		default:
			tp = mktype(tp, bp->op, bp->nelem, bp->data);
			break;
		}
	}

	if ((name = sym->name) == NULL) {
		sym->type = tp;
	} else {
		short flags;

		if ((sym = install(ns, osym = sym)) == NULL) {
			if (!eqtype(osym->type, tp))
				error("conflicting types for '%s'", name);
			sym = osym;
		} else {
			sym->u.pars = pars;
			sym->type = tp;
		}
		if (!tp->defined && sclass != EXTERN) {
			error("declared variable '%s' of incomplete type",
			      name);
		}
	}

	return sym;
}

static Type *structdcl(void), *enumdcl(void);

static Type *
specifier(int *sclass)
{
	Type *tp = NULL;
	unsigned spec, qlf, sign, type, cls, size;

	spec = qlf = sign = type = cls = size = 0;

	for (;;) {
		unsigned *p;
		Type *(*dcl)(void) = NULL;

		switch (yytoken) {
		case SCLASS:
			p = &cls;
			break;
		case TQUALIFIER:
			if ((qlf |= yylval.token) & RESTRICT)
				goto invalid_type;
			next();
			continue;
		case TYPEIDEN:
			if (type)
				goto return_type;
			tp = yylval.sym->type;
			p = &type;
			break;
		case TYPE:
			switch (yylval.token) {
			case ENUM:
				dcl = enumdcl;
				p = &type;
				break;
			case STRUCT:
			case UNION:
				dcl = structdcl;
				p = &type;
				break;
			case VOID:
			case BOOL:
			case CHAR:
			case INT:
			case FLOAT:
			case DOUBLE:
				p = &type;
				break;
			case SIGNED:
			case UNSIGNED:
				p = &sign;
				break;
			case LONG:
				if (size == LONG) {
					size = LLONG;
					break;
				}
			case SHORT:
				p = &size;
				break;
			}
			break;
		default:
			goto return_type;
		}
		if (*p)
			goto invalid_type;
		*p = yylval.token;
		if (dcl) {
			if (size || sign)
				goto invalid_type;
			tp = (*dcl)();
			goto return_type;
		} else {
			next();
		}
		spec = 1;
	}

return_type:
	if (sclass)
		*sclass = cls;
	if (!tp && spec)
		tp = ctype(type, sign, size);
	return tp;

invalid_type:
	error("invalid type specification");
}

/* TODO: check correctness of the initializator  */
/* TODO: emit initializer */
static struct node *
initializer(Symbol *sym)
{
	if (!(sym->flags & ISEXTERN))
		error("'%s' initialized and declared extern", sym->name);

	if (accept('{')) {
		initializer(sym);
		expect('}');
	} else {
		do {
			expr();
		} while (accept(','));
	}
	return NULL;
}

static Symbol *
newtag(void)
{
	Symbol *sym;
	int op, tag = yylval.token;
	static unsigned ns = NS_STRUCTS;

	setnamespace(NS_TAG);
	next();
	switch (yytoken) {
	case IDEN:
	case TYPEIDEN:
		sym = yylval.sym;
		if ((sym->flags & ISDEFINED) == 0)
			install(NS_TAG, yylval.sym);
		next();
		break;
	default:
		sym = newsym(NS_TAG);
		break;
	}
	if (!sym->type) {
		if (ns == NS_STRUCTS + NR_MAXSTRUCTS)
			error("too much tags declared");
		sym->type = mktype(NULL, tag, 0, NULL);
		sym->type->ns = ns++;
	}

	if ((op = sym->type->op) != tag &&  op != INT)
		error("'%s' defined as wrong kind of tag", sym->name);
	return sym;
}

/* TODO: bitfields */

static void fieldlist(Type *tp);

static Type *
structdcl(void)
{
	Symbol *sym;
	Type *tp;

	sym = newtag();
	tp = sym->type;
	if (!accept('{'))
		return tp;

	if (tp->defined)
		error("redefinition of struct/union '%s'", sym->name);
	tp->defined = 1;

	while (!accept('}'))
		fieldlist(tp);
	return tp;
}

static Type *
enumdcl(void)
{
	Type *tp;
	Symbol *sym, *tagsym;
	int val;

	tagsym = newtag();
	tp = tagsym->type;

	if (!accept('{'))
		return tp;
	if (tp->defined)
		error("redefinition of enumeration '%s'", tagsym->name);
	tp->defined = 1;
	for (val = 0; yytoken != ')'; ++val) {
		if (yytoken != IDEN)
			unexpected();
		if ((sym = install(NS_IDEN, yylval.sym)) == NULL) {
			error("'%s' redeclared as different kind of symbol",
			      yytext);
		}
		next();
		sym->flags |= ISCONSTANT;
		sym->type = inttype;
		if (accept('=')) {
			Node *np = iconstexpr();

			if (np == NULL)
				error("invalid enumeration value");
			val = np->sym->u.i;
			freetree(np);
		}
		sym->u.i = val;
		if (!accept(','))
			break;
	}
	expect('}');

	return tp;
}

static void
type(Symbol *sym, int sclass, Type *data)
{
	if (sclass)
		error("class storage in type name");
	if (sym->name)
		error("unexpected identifier in type name");
}

static void
field(Symbol *sym, int sclass, Type *data)
{
	Type *tp = sym->type, *funtp = data;
	size_t n = funtp->n.elem;

	if (sclass)
		error("storage class in struct/union field");
	if (!sym->name)
		error("missed identifier in field declaration");
	if (tp->op == FTN)
		error("invalid type in struct/union");
	sym->flags |= ISFIELD;
	if (n++ == NR_FUNPARAM)
		error("too much fields in struct/union");
	funtp->pars = xrealloc(funtp->pars, n);
	funtp->pars[n-1] = tp;
	funtp->n.elem = n;
}

static void
internal(Symbol *sym, int sclass, Type *data)
{
	if (!sym->name) {
		warn("empty declaration");
		return;
	}
	if (sym->type->op != FTN) {
		if (!sclass)
			sym->flags |= ISAUTO;
		if (accept('='))
			initializer(sym);
		/* TODO: check if the variable is extern and has initializer */
	}
	if (sym->flags & ISSTATIC)
		sym->flags |= ISLOCAL;
	emit(ODECL, sym);
}

static void
external(Symbol *sym, int sclass, Type *data)
{
	if (!sym->name) {
		warn("empty declaration");
		return;
	}

	if (sym->flags & (ISREGISTER|ISAUTO))
		error("incorrect storage class for file-scope declaration");
	sym->flags |= (sym->flags & ISSTATIC) ? ISPRIVATE : ISGLOBAL;

	if (sym->type->op == FTN && yytoken == '{') {
		if (sym->token == TYPEIDEN)
			error("function definition declared 'typedef'");
		curfun = sym;
		sym->flags |= ISDEFINED;
		emit(OFUN, sym);
		compound(NULL, NULL, NULL);
		emit(OEFUN, NULL);
		return;
	}
	if (accept('='))
		initializer(sym);
	/* TODO: check if the variable is extern and has initializer */
	emit(ODECL, sym);
}

static Symbol *
dodcl(int rep, void (*fun)(Symbol *, int, Type *), uint8_t ns, Type *type)
{
	Symbol *sym;
	Type *base, *tp;
	int sclass;

	if ((base = specifier(&sclass)) == NULL) {
		if (curctx != 0)
			unexpected();
		warn("type defaults to 'int' in declaration");
		base = inttype;
	}

	do {
		sym = declarator(base, ns, sclass);
		tp = sym->type;

		switch (sclass) {
		case REGISTER:
			sym->flags |= ISREGISTER;
			break;
		case AUTO:
			sym->flags |= ISAUTO;
			break;
		case STATIC:
			sym->flags |= ISSTATIC;
			break;
		case EXTERN:
			sym->flags |= ISEXTERN;
			break;
		case TYPEDEF:
			sym->token = TYPEIDEN;
			break;
		}
		if (tp->op == FTN && (sym->flags & (ISREGISTER|ISAUTO))) {
			error("invalid storage class for function '%s'",
			      sym->name);
		}
		(*fun)(sym, sclass, type);
	} while (rep && !curfun && accept(','));

	return sym;
}

void
decl(void)
{
	if (accept(';'))
		return;
	if (!dodcl(1, (curctx == 0) ? external : internal, NS_IDEN, NULL))
		return;
	if (curfun)
		curfun == NULL;
	else
		expect(';');
}

static void
fieldlist(Type *tp)
{
	if (yytoken != ';')
		dodcl(1, field, tp->ns, tp);
	expect(';');
}

Type *
typename(void)
{
	return dodcl(0, type, 0, NULL)->type;
}
