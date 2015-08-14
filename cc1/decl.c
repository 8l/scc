
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define NOSCLASS  0

struct declarators {
	unsigned char nr;
	struct declarator {
		unsigned char op;
		unsigned short nelem;
		Symbol *sym;
		Type **tpars;
		Symbol **pars;
	} d [NR_DECLARATORS];
};

struct decl {
	unsigned ns;
	int sclass;
	Symbol *sym;
	Symbol **pars;
	Type *type;
	Type *parent;
};

static void
push(struct declarators *dp, unsigned op, ...)
{
	va_list va;
	unsigned n;
	struct declarator *p;

	va_start(va, op);
	if ((n = dp->nr++) == NR_DECLARATORS)
		error("too much declarators");

	p = &dp->d[n];
	p->op = op;
	p->tpars = NULL;

	switch (op) {
	case ARY:
		p->nelem = va_arg(va, unsigned);
		break;
	case FTN:
		p->nelem = va_arg(va, unsigned);
		p->tpars = va_arg(va, Type **);
		p->pars = va_arg(va, Symbol **);
		break;
	case IDEN:
		p->sym = va_arg(va, Symbol *);
		break;
	}
	va_end(va);
}

static bool
pop(struct declarators *dp, struct decl *dcl)
{
	struct declarator *p;

	if (dp->nr == 0)
		return 0;

	p = &dp->d[--dp->nr];
	if (p->op == IDEN) {
		dcl->sym = p->sym;
		return 1;
	}
	if (dcl->type->op == FTN) {
		/*
		 * constructor applied to a
		 * function. We  don't need
		 * the parameter symbols anymore.
		 */
		free(dcl->pars);
		popctx();
		dcl->pars = NULL;
	}
	if (p->op == FTN)
		dcl->pars = p->pars;
	dcl->type = mktype(dcl->type, p->op, p->nelem, p->tpars);
	return 1;
}

static void
arydcl(struct declarators *dp)
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

	push(dp, ARY, n);
}

static Symbol *
parameter(struct decl *dcl)
{
	Symbol *sym = dcl->sym;
	Type *funtp = dcl->parent, *tp = dcl->type;
	size_t n = funtp->n.elem;
	char *name = sym->name;

	sym->type = tp;

	if (n == -1)
		error("'void' must be the only parameter");

	switch (dcl->sclass) {
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
		if (dcl->sclass)
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
	sym->flags |= ISUSED;    /* avoid non used warnings in prototypes */

	if (n++ == NR_FUNPARAM)
		error("too much parameters in function definition");
	funtp->p.pars = xrealloc(funtp->p.pars, n * sizeof(Type *));
	funtp->p.pars[n-1] = tp;
	funtp->n.elem = n;

	return sym;
}

static Symbol *dodcl(int rep,
                     Symbol *(*fun)(struct decl *),
                     unsigned ns,
                     Type *type);

static void
fundcl(struct declarators *dp)
{
	Type type = {.n = {.elem = -1}, .p = {.pars= NULL}};
	Symbol *syms[NR_FUNPARAM], **sp;
	size_t size;
	Symbol *pars = NULL;

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
	push(dp, FTN, type.n.elem, type.p.pars, pars);
}

static void declarator(struct declarators *dp, unsigned ns);

static void
directdcl(struct declarators *dp, unsigned ns)
{
	Symbol *sym;

	if (accept('(')) {
		declarator(dp, ns);
		expect(')');
	} else {
		if (yytoken == IDEN || yytoken == TYPEIDEN) {
			sym = yylval.sym;
			next();
		} else {
			sym = newsym(ns);
		}
		push(dp, IDEN, sym);
	}

	for (;;) {
		switch (yytoken) {
		case '(':  fundcl(dp); break;
		case '[':  arydcl(dp); break;
		default:   return;
		}
	}
}

static void
declarator(struct declarators *dp, unsigned ns)
{
	unsigned  n;

	for (n = 0; accept('*'); ++n) {
		while (accept(TQUALIFIER))
			/* nothing */;
	}

	directdcl(dp, ns);

	while (n--)
		push(dp, PTR);
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
	if (!tp) {
		if (spec) {
			tp = ctype(type, sign, size);
		} else {
			if (curctx != GLOBALCTX)
				unexpected();
			warn("type defaults to 'int' in declaration");
			tp = inttype;
		}
	}
	return tp;

invalid_type:
	error("invalid type specification");
}

/* TODO: check correctness of the initializator  */
/* TODO: emit initializer */
static struct node *
initializer(Symbol *sym)
{
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
		Type *tp;

		if (ns == NS_STRUCTS + NR_MAXSTRUCTS)
			error("too much tags declared");
		tp = mktype(NULL, tag, 0, NULL);
		tp->ns = ns++;
		tp->p.fields = NULL;
		sym->type = tp;
		tp->tag = sym;
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
type(struct decl *dcl)
{
	Symbol *sym = dcl->sym;

	if (dcl->sclass)
		error("class storage in type name");
	if (sym->name)
		error("unexpected identifier in type name");
	sym->type = dcl->type;

	return sym;
}

static Symbol *
field(struct decl *dcl)
{
	Symbol *sym = dcl->sym;
	char *name = sym->name;
	Type *structp = dcl->parent, *tp = dcl->type;
	size_t n = structp->n.elem;

	if (!name) {
		sym->type = tp;
		warn("empty declaration");
		return sym;
	}
	if (dcl->sclass)
		error("storage class in struct/union field");
	if (tp->op == FTN)
		error("invalid type in struct/union");
	if (!tp->defined)
		error("field '%s' has incomplete type", name);

	if ((sym = install(dcl->ns, sym)) == NULL)
		error("duplicated member '%s'", name);
	sym->type = tp;

	sym->flags |= ISFIELD;
	if (n++ == NR_FUNPARAM)
		error("too much fields in struct/union");
	structp->p.fields = xrealloc(structp->p.fields, n * sizeof(*sym));
	structp->p.fields[n-1] = sym;
	structp->n.elem = n;

	return sym;
}

static Symbol *
identifier(struct decl *dcl)
{
	Symbol *sym = dcl->sym;
	Type *tp = dcl->type;
	char *name = sym->name;
	short flags;
	int sclass = dcl->sclass;

	if (!name) {
		sym->type = tp;
		switch (tp->op) {
		default:
			warn("empty declaration");
		case STRUCT:
		case UNION:
		case ENUM:
			return sym;
		}
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
		sym = install(NS_IDEN, sym);
	}

	if (sym == NULL) {
		sym = dcl->sym;
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

	sym->u.pars = dcl->pars;
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
dodcl(int rep, Symbol *(*fun)(struct decl *), unsigned ns, Type *parent)
{
	Symbol *sym;
	Type *base;
	struct decl dcl;
	struct declarators stack;

	dcl.ns = ns;
	dcl.parent = parent;
	base = specifier(&dcl.sclass);

	do {
		stack.nr = 0;
		dcl.pars = NULL;
		dcl.type = base;

		declarator(&stack, ns);

		while (pop(&stack, &dcl))
			/* nothing */;
		sym = (*fun)(&dcl);
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
