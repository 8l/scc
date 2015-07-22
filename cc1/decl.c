
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define ID_EXPECTED     1
#define ID_ACCEPTED     2
#define ID_FORBIDDEN    3

/* TODO: check identifiers in enum declaration */

struct dcldata {
	unsigned char op;
	unsigned short nelem;
	unsigned ndcl;
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
	Node *np;
	TINT n;

	expect('[');
	np = (yytoken != ']') ? constexpr() : NULL;
	expect(']');

	/*
	 * TODO: check that the type of the constant expression
	 * is the correct, that in this case should be int
	 */
	n = (np == NULL) ? 0 : np->sym->u.i;
	freetree(np);

	return queue(dp, ARY, n, NULL);
}

static Symbol *parameter(void);

static struct dcldata *
fundcl(struct dcldata *dp)
{
	size_t siz;
	unsigned n, i, noname;
	Type *pars[NR_FUNPARAM], **tp = pars;
	Symbol *syms[NR_FUNPARAM], **sp = syms, *sym;

	pushctx();
	expect('(');

	n = noname = 0;
	do {
		if ((sym = parameter()) == NULL) {
			if (n == 0)
				break;
			error("incorrect void parameter");
		}
		if (n++ == NR_FUNPARAM)
			error("too much parameters in function definition");
		*sp++ = sym;
		*tp++ = sym->type;
		noname |= sym->name[0] == '\0';
	} while (accept(','));

	expect(')');
	if (n != 0) {
		siz = sizeof(*tp) * n;
		tp = memcpy(xmalloc(siz), pars, siz);
	} else {
		tp = NULL;
	}

	if (yytoken != '{') {
		/* it is only a prototype */
		popctx();
	} else {
		/* it is a function definition */
		if (noname)
			error("parameter name omitted");
		sp = syms;
		for (i = 0; i < n; ++i)
			emit(ODECL, *sp++);
	}

	return queue(dp, FTN, n, tp);
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
declarator(Type *tp, int flags, unsigned ns)
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
			if (flags == ID_EXPECTED && *sym->name == '\0')
				error("missed identifier in declaration");
			if (flags == ID_FORBIDDEN && *sym->name != '\0')
				error("unexpected identifier in type name");
			break;
		}
	}

	/* TODO: deal with external array declarations of []  */
	if (!tp->defined && *sym->name)
		error("declared variable '%s' of incomplete type", sym->name);
	sym->type = tp;
	return sym;
}

static Type *structdcl(void), *enumdcl(void);

static Type *
specifier(unsigned *sclass)
{
	Type *tp = NULL;
	unsigned qlf, sign, type, cls, size;

	qlf = sign = type = cls = size = 0;

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
	}

return_type:
	if (sclass)
		*sclass = cls;
	if (!tp)
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
	unsigned tag = yylval.token;
	static unsigned ns = NS_STRUCTS;

	setnamespace(NS_TAG);
	next();
	switch (yytoken) {
	case IDEN:
	case TYPEIDEN:
		sym = yylval.sym;
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

	sym->flags |= ISDEFINED;
	if (sym->type->op != tag)
		error("'%s' defined as wrong kind of tag", yytext);
	return sym;
}

/* TODO: bitfields */

static Type *
structdcl(void)
{
	Type *tagtype, *buff[NR_MAXSTRUCTS], **bp = &buff[0];
	Symbol *tagsym, *sym;
	unsigned n;
	size_t siz;

	tagsym = newtag();
	tagtype = tagsym->type;
	if (!accept('{'))
		return tagtype;

	if (tagtype->defined)
		error("redefinition of struct/union '%s'", yytext);
	tagtype->defined = 1;
	emit(OSTRUCT, tagsym);

	while (!accept('}')) {
		Type *base, *tp;

		switch (yytoken) {
		case SCLASS:
			error("storage class '%s' in struct/union field",
			      yytext);
		case IDEN: case TYPE: case TYPEIDEN: case TQUALIFIER:
			base = specifier(NULL);
			break;
		case ';':
			next();
			continue;
		default:
			unexpected();
		}

		if (accept(';'))
			error("identifier expected");

		do {
			sym = declarator(base, ID_EXPECTED, tagtype->ns);
			sym->flags |= ISFIELD;
			tp = sym->type;
			if (tp->op == FTN)
				error("invalid type in struct/union");
			if (bp == &buff[NR_MAXSTRUCTS])
				error("too much fields in struct/union");
			*bp++ = sym->type;
			emit(ODECL, sym);
		} while (accept(','));
		expect(';');
	}

	emit(OESTRUCT, NULL);
	if ((n = bp - buff) != 0) {
		siz = sizeof(Type *) * n;
		tagtype->n.elem = n;
		tagtype->pars = memcpy(xmalloc(siz), buff, siz);
	}
	return tagtype;
}

static Type *
enumdcl(void)
{
	Type *tp;
	Symbol *sym;
	int val = 0;

	tp = newtag()->type;

	if (yytoken == ';')
		return tp;

	expect('{');
	if (tp->defined)
		error("redefinition of enumeration '%s'", yytext);
	tp->defined = 1;
	while (yytoken != '}') {
		if (yytoken != IDEN)
			unexpected();
		if ((sym = install(NS_IDEN)) == NULL)
			error("'%s' redeclared as different kind of symbol", yytext);
		next();
		sym->type = inttype;
		if (accept('='))
			constexpr();
		sym->u.i = val++;
		if (!accept(','))
			break;
	}
	expect('}');

	return tp;
}

static Symbol *
parameter(void)
{
	Symbol *sym;
	unsigned sclass;
	Type *tp;

	if ((tp = specifier(&sclass)) == voidtype)
		return NULL;
	sym = declarator(tp, ID_ACCEPTED, NS_IDEN);
	sym->flags |= ISPARAM;
	tp = sym->type;
	if (tp->op == FTN)
		error("incorrect function type for a function parameter");
	if (tp->op == ARY)
		tp = mktype(tp->type, PTR, 0, NULL);
	switch (sclass) {
	case REGISTER:
		sym->flags |= ISREGISTER;
		break;
	case 0:
		sym->flags |= ISAUTO;
		break;
	default:
		error("bad storage class in function parameter");
	}
	return sym;
}

void
decl(void)
{
	Type *tp;
	Symbol *sym;
	unsigned sclass, isfun;
	extern jmp_buf recover;

	setsafe(END_DECL);
	if (setjmp(recover))
		return;
	tp = specifier(&sclass);
	if (accept(';'))
		return;

	do {
		setsafe(END_LDECL);
		setjmp(recover);
		sym = declarator(tp, ID_EXPECTED, NS_IDEN);
		isfun = sym->type->op == FTN;

		switch (sclass) {
		case TYPEDEF:
			sym->token = TYPEIDEN;
			continue;
		case STATIC:
			sym->flags |= ISSTATIC;
			break;
		case EXTERN:
			sym->flags |= ISEXTERN;
			break;
		case REGISTER:
			sym->flags = ISREGISTER;
			if (isfun)
				goto bad_function;
			break;
		case AUTO:
			if (isfun)
				goto bad_function;
			/* passtrough */
		default:
			sym->flags |= ISAUTO;
			break;
		}
		if (accept('='))
			initializer(sym);
		emit(ODECL, sym);
	} while (accept(','));

	expect(';');
	return;

bad_function:
	error("invalid storage class for function '%s'", sym->name);
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
	sym = declarator(tp, ID_FORBIDDEN, NS_IDEN);
	return  sym->type;
}

void
extdecl(void)
{
	Type *base, *tp;
	unsigned sclass;
	Symbol *sym;
	extern Symbol *curfun;
	extern jmp_buf recover;

	setsafe(END_DECL);
	if (setjmp(recover))
		return;

	switch (yytoken) {
	case IDEN: case TYPE: case TYPEIDEN: case SCLASS: case TQUALIFIER:
		base = specifier(&sclass);
		if (accept(';'))
			return;
		do {
			/* FIX: we cannot put a setjmp here because
			   base was already assigned, and we were having
			   problems with EOF */
			sym = declarator(base, ID_EXPECTED, NS_IDEN);
			tp = sym->type;
			sym->flags |= ISSTATIC;
			sym->flags |= ISGLOBAL;

			switch (sclass) {
			case REGISTER: case AUTO:
				error("incorrect storage class for file-scope declaration");
			case STATIC:
				sym->flags |= ISSTATIC;
				break;
			case EXTERN:
				sym->flags |= ISEXTERN;
				break;
			case TYPEDEF:
				sym->token = TYPEIDEN;
				continue;
			}

			if (tp->op != FTN) {
				if (accept('='))
					initializer(sym);
				emit(ODECL, sym);
			} else if (yytoken == '{') {
				curfun = sym;
				emit(OFUN, sym);
				compound(NULL, NULL, NULL);
				emit(OEFUN, NULL);
				popctx();
				return;
			}
		} while (accept(','));
		/* PASSTHROUGH */
	case ';':
		expect(';');
		return;
	default:
		unexpected();
	}
}

