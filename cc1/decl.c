
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define OUTCTX 0
#define PARCTX 1

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

static void parlist(Type *);

static struct dcldata *
fundcl(struct dcldata *dp)
{
	Type dummy = {.n = {.elem = 0}, .pars = NULL};

	parlist(&dummy);

	return queue(dp, FTN, dummy.n.elem, dummy.pars);
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
			if ((sym = install(ns)) == NULL)
				error("redeclaration of '%s'", yytext);
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
declarator(Type *tp, unsigned ns)
{
	struct dcldata data[NR_DECLARATORS+1];
	struct dcldata *bp;
	Symbol *sym;

	data[0].ndcl = 0;
	for (bp = declarator0(data, ns); bp > data; ) {
		--bp;
		if (bp->op != IDEN) {
			tp = mktype(tp, bp->op, bp->nelem, bp->data);
		} else {
			sym = bp->data;
			break;
		}
	}

	/* TODO: deal with external array declarations of []  */
	if (!tp->defined && sym->name)
		error("declared variable '%s' of incomplete type", sym->name);
	sym->type = tp;
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
			install(NS_TAG);
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
		if ((sym = install(NS_IDEN)) == NULL)
			error("'%s' redeclared as different kind of symbol", yytext);
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

Type *
typename(void)
{
	unsigned sclass;
	Type *tp;
	Symbol *sym;

	tp = specifier(&sclass);
	if (sclass)
		error("class storage in type name");
	sym = declarator(tp, NS_IDEN);

	if (!sym->name)
		error("unexpected identifier in type name");

	return  sym->type;
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
	if (!sclass)
		sym->flags |= ISAUTO;
	if (sym->flags & (ISSTATIC|ISEXTERN))
		error("bad storage class in function parameter");
	if (n++ == NR_FUNPARAM)
		error("too much parameters in function definition");
	sym->flags |= ISPARAM;
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
	if (!sclass)
		sym->flags |= ISAUTO;
	if (accept('='))
		initializer(sym);
	/* TODO: check if the variable is extern and has initializer */
	emit(ODECL, sym);
}

static void
external(Symbol *sym, int sclass, Type *data)
{
	if (!sym->name) {
		warn("empty declaration");
		return;
	}
	sym->flags |= ISSTATIC|ISGLOBAL;

	if (sym->flags & (ISREGISTER|ISAUTO))
		error("incorrect storage class for file-scope declaration");
	if (accept('='))
		initializer(sym);
	/* TODO: check if the variable is extern and has initializer */
	emit(ODECL, sym);
}

static int
prototype(Symbol *sym)
{
	int r = 1;

	/* TODO: check type of the function */
	/* TODO: check function is not redefined */

	if (sym->flags & (ISREGISTER|ISAUTO))
		error("invalid storage class for function '%s'", sym->name);

	if (curctx == PARCTX && yytoken == '{') {
		if (sym->token == TYPEIDEN)
			error("function definition declared 'typedef'");

		sym->flags |= ISDEFINED;
		curfun = sym;
		emit(OFUN, sym);
		compound(NULL, NULL, NULL);
		emit(OEFUN, NULL);
		popctx();
		r = 0;
	}

	return r;
}

static Symbol *
dodcl(int rep, void (*fun)(Symbol *, int, Type *), uint8_t ns, Type *type)
{
	Symbol *sym;
	Type *base, *tp;
	int sclass;

	/* FIXME: curctx == PARCTX is incorrect. Structs also
	 * create new contexts
	 */
	if ((base = specifier(&sclass)) == NULL) {
		if (curctx != OUTCTX)
			unexpected();
		warn("type defaults to 'int' in declaration");
		base = inttype;
	}

	do {
		sym = declarator(base, ns);
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
		if (tp->op == FTN && !prototype(sym))
			return NULL;
		(*fun)(sym, sclass, type);

	} while (rep && accept(','));

	return sym;
}

void
decl(void)
{
	if (accept(';'))
		return;
	if (!dodcl(1, curctx == OUTCTX ? external : internal, NS_IDEN, NULL))
		return;
	expect(';');
}

/*
 * parlist() is called every time there is a argument list.
 * It means that is called for prototypes and for functions.
 * In both cases a new context is needed for the arguments,
 * but in the case of prototypes we need pop the context
 * before parsing anything else or we can have name conflicts.
 * The heuristic used here to detect a function is check if
 * next token will be '{', but it implies that K&R alike
 * functions are not allowed.
 */
static void
parlist(Type *tp)
{
	Symbol *pars[NR_FUNPARAM], **sp = pars;
	bool isfun;
	int n;

	pushctx();
	expect('(');

	if (accept(')')) {
		tp->n.elem = -1;
		return;
	}

	do
		*sp++ = dodcl(0, parameter, NS_IDEN, tp);
	while (accept(','));

	isfun = ahead() == '{';
	if (!isfun)
		popctx();
	expect(')');

	if (!isfun)
		return;

	n = tp->n.elem;
	for (sp = pars; n-- > 0; ++sp)
		emit(ODECL, *sp);
}

static void
fieldlist(Type *tp)
{
	if (yytoken != ';')
		dodcl(1, field, tp->ns, tp);
	expect(';');
}
