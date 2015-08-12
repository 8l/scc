
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define NOSCLASS  0

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

static Symbol *
parameter(Symbol *sym, Type *tp, unsigned ns, int sclass, Type *data)
{
	Type *funtp = data;
	size_t n = funtp->n.elem;
	char *name = sym->name;

	sym->type = tp;

	if (n == -1)
		error("'void' must be the only parameter");

	switch (sclass) {
	case STATIC:
	case EXTERN:
	case AUTO:
		error("bad storage class in function parameter");
	case REGISTER:
		sym->flags |= ISREGISTER;
		break;
	case NOSCLASS:
		sym->flags |= ISAUTO;
		break;
	}

	switch (tp->op) {
	case VOID:
		if (n != 0)
			error("incorrect void parameter");
		if (sclass)
			error("void as unique parameter may not be qualified");
		funtp->n.elem = -1;
		return NULL;
	case ARY:
		tp = mktype(tp->type, PTR, 0, NULL);
		break;
	case FTN:
		error("incorrect function type for a function parameter");
	}

	if (name) {
		if ((sym = install(NS_IDEN, sym)) == NULL)
			error("redefinition of parameter '%s'", name);
	}
	sym->type = tp;

	if (n++ == NR_FUNPARAM)
		error("too much parameters in function definition");
	funtp->pars = xrealloc(funtp->pars, n * sizeof(Type *));
	funtp->pars[n-1] = tp;
	funtp->n.elem = n;

	return sym;
}

static Symbol *dodcl(int rep,
                     Symbol *(*fun)(Symbol *, Type *, unsigned, int, Type *),
                     unsigned ns,
                     Type *type);

static struct dcldata *
fundcl(struct dcldata *dp)
{
	Type type = {.n = {.elem = -1}, .pars = NULL};
	Symbol *syms[NR_FUNPARAM], **sp;
	size_t size;
	void *pars = NULL;

	pushctx();
	expect('(');

	if (!accept(')')) {
		type.n.elem = 0;
		sp = syms;
		do
			*sp++ = dodcl(0, parameter, NS_IDEN, &type);
		while (accept(','));

		expect(')');

		if (type.n.elem != -1) {
			size = type.n.elem * sizeof(Symbol *);
			pars = memcpy(xmalloc(size), syms, size);
		}
	}
	dp = queue(dp, PARS, 0, pars);
	dp = queue(dp, FTN, type.n.elem, type.pars);

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
declarator(Type *tp, unsigned ns, Type **otp)
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
			if (pars) {
				/*
				 * constructor applied to a function. We  don't
				 * need the parameter symbols anymore.
				 */
				free(pars);
				popctx();
				pars = NULL;
			}
			tp = mktype(tp, bp->op, bp->nelem, bp->data);
			break;
		}
	}
	/*
	 * FIXME: This assignation can destroy pars of a previous definition
	 */
	if (pars)
		sym->u.pars = pars;
	*otp = tp;
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
		if ((sym->flags & ISDECLARED) == 0)
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

static Symbol *
type(Symbol *sym, Type *tp, unsigned ns, int sclass, Type *data)
{
	if (sclass)
		error("class storage in type name");
	if (sym->name)
		error("unexpected identifier in type name");
	sym->type = tp;

	return sym;
}

static Symbol *
field(Symbol *sym, Type *tp, unsigned ns, int sclass, Type *data)
{
	Type *funtp = data;
	size_t n = funtp->n.elem;
	char *name = sym->name;

	if (!name) {
		sym->type = tp;
		warn("empty declaration");
		return sym;
	}
	if (sclass)
		error("storage class in struct/union field");
	if (tp->op == FTN)
		error("invalid type in struct/union");
	if (!tp->defined)
		error("field '%s' has incomplete type", name);

	if ((sym = install(ns, sym)) == NULL)
		error("duplicated member '%s'", name);
	sym->type = tp;

	sym->flags |= ISFIELD;
	if (n++ == NR_FUNPARAM)
		error("too much fields in struct/union");
	funtp->pars = xrealloc(funtp->pars, n);
	funtp->pars[n-1] = tp;
	funtp->n.elem = n;

	return sym;
}

static Symbol *
identifier(Symbol *sym, Type *tp, unsigned ns, int sclass, Type *data)
{
	char *name = sym->name;
	short flags;
	Symbol *osym;

	if (!name) {
		sym->type = tp;
		warn("empty declaration");
		return sym;
	}

	/* TODO: Add warning about ANSI limits */
	if (!tp->defined && sclass != EXTERN)
		error("declared variable '%s' of incomplete type", name);

	if (tp->op == FTN) {
		if (sclass == NOSCLASS)
			sclass = EXTERN;
		/*
		 * Ugly workaround to solve function declarations.
		 * A new context is added for the parameters,
		 * so at this point curctx is incremented by
		 * one when sym was parsed.
		 */
		--curctx;
		sym = install(NS_IDEN, sym);
		++curctx;
	} else {
		sym = install(NS_IDEN, osym = sym);
	}

	if (sym == NULL) {
		sym = osym;
		flags = sym->flags;
		if (!eqtype(sym->type, tp))
			error("conflicting types for '%s'", name);
		if (sym->token == TYPEIDEN && sclass != TYPEDEF ||
		    sym->token != TYPEIDEN && sclass == TYPEDEF)
				goto redeclaration;

		if (curctx != GLOBALCTX && tp->op != FTN) {
			if (!(sym->flags & ISEXTERN) || sclass != EXTERN)
				goto redeclaration;
		} else {
			switch (sclass) {
			case REGISTER:
			case AUTO:
				goto bad_storage;
			case NOSCLASS:
				if (flags & ISPRIVATE)
					goto non_after_static;
				flags &= ~ISEXTERN;
				flags |= ISGLOBAL;
			case TYPEDEF:
			case EXTERN:
				break;
			case STATIC:
				if (flags & (ISGLOBAL|ISEXTERN))
					goto static_after_non;
				flags |= ISPRIVATE;
				break;
			}
		}
	} else {
		flags = sym->flags;
		switch (sclass) {
		case REGISTER:
		case AUTO:
			if (curctx == GLOBALCTX || tp->op == FTN)
				goto bad_storage;
			flags |= (sclass == REGISTER) ? ISREGISTER : ISAUTO;
			break;
		case NOSCLASS:
			flags |= (curctx == GLOBALCTX) ? ISGLOBAL : ISAUTO;
			break;
		case EXTERN:
			flags |= ISEXTERN;
			break;
		case STATIC:
			flags |= (curctx == GLOBALCTX) ? ISPRIVATE : ISLOCAL;
			break;
		case TYPEDEF:
			sym->token = TYPEIDEN;
			break;
		}
	}

	sym->flags = flags;
	sym->type = tp;

	if (accept('='))
		initializer(sym);
	/* TODO: disallow initializators in functions */
	/* TODO: check if typedef has initializer */
	/* TODO: check if the variable is extern and has initializer */
	if (sym->token == IDEN)
		emit(ODECL, sym);
	return sym;

redeclaration:
	error("redeclaration of '%s'", name);

bad_storage:
	if (tp->op != FTN)
		error("incorrect storage class for file-scope declaration");
bad_function:
	error("invalid storage class for function '%s'", name);

non_after_static:
	error("non-static declaration of '%s' follows static declaration",
	      name);

static_after_non:
	error("static declaration of '%s' follows non-static declaration",
	      name);
}

static Symbol *
dodcl(int rep,
      Symbol *(*fun)(Symbol *, Type *, unsigned, int, Type *),
      unsigned ns,
      Type *data)
{
	Symbol *sym;
	Type *base, *tp;
	int sclass;

	if ((base = specifier(&sclass)) == NULL) {
		if (curctx != GLOBALCTX)
			unexpected();
		warn("type defaults to 'int' in declaration");
		base = inttype;
	}

	do {
		sym = declarator(base, ns, &tp);
		sym = (*fun)(sym, tp, ns, sclass, data);
	} while (rep && accept(','));

	return sym;
}

void
decl(void)
{
	Symbol *sym;

	if (accept(';'))
		return;
	sym = dodcl(1, identifier, NS_IDEN, NULL);

	/*
	 * Functions only can appear at global context,
	 * but due to parameter context, we have to check
	 * against GLOBALCTX+1
	 */
	if (sym->type->op == FTN) {
		if (curctx != GLOBALCTX+1)
			goto remove_pars;

		switch (yytoken) {
		case '{':
		case TYPEIDEN:
		case TYPE:
		case TQUALIFIER:
		case SCLASS:
			if (sym->token == TYPEIDEN)
				error("function definition declared 'typedef'");
			if (sym->flags & ISDEFINED)
				error("redefinition of '%s'", sym->name);
			if (sym->flags & ISEXTERN) {
				sym->flags &= ~ISEXTERN;
				sym->flags |= ISGLOBAL;
			}
			sym->flags |= ISDEFINED;
			sym->flags &= ~ISEMITTED;
			curfun = sym;
			emit(OFUN, sym);
			free(sym->u.pars);
			compound(NULL, NULL, NULL);
			emit(OEFUN, NULL);
			curfun = NULL;
			return;
		default:
		remove_pars:
			free(sym->u.pars);
			popctx();
		}
	}
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
