#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sizes.h>
#include <cc.h>
#include "cc1.h"

#define ID_EXPECTED     1
#define ID_ACCEPTED     2
#define ID_FORBIDDEN    3

/* TODO: check identifiers in enum declaration */

struct dcldata {
	uint8_t op;
	unsigned short nelem;
	void *data;
};

static struct dcldata *
queue(struct dcldata *dp, uint8_t op, short nelem, void *data)
{
	if (dp->op == 255)
		error("too much declarators");
	dp->op = op;
	dp->nelem = nelem;
	dp->data = data;
	return dp + 1;
}

static struct dcldata *
arydcl(struct dcldata *dp)
{
	expect('[');
	expect(']');
	return queue(dp, ARY, 0, NULL);
}

static Type *parameter(void);

static struct dcldata *
fundcl(struct dcldata *dp)
{
	uint8_t n = 0;
	size_t siz;
	Type *pars[NR_FUNPARAM], **tp = &pars[0];

	expect('(');

	do {
		if (tp == &pars[NR_FUNPARAM])
			error("too much parameters in function definition");

		if ((*tp++ = parameter()) == voidtype) {
			if (n == 0)
				break;
			else
				error("incorrect void parameter");
		}
		++n;
	} while (accept(','));

	expect(')');
	siz = sizeof(*tp) * n;
	tp = (siz > 0) ? memcpy(xmalloc(siz), pars, siz) : NULL;

	return queue(dp, FTN, n, tp);
}

static Symbol *
newiden(uint8_t ns)
{
	Symbol *sym;
	extern uint8_t curctx;

	if (yylval.sym && yylval.sym->ctx == curctx && yylval.sym->ns == ns)
		error("redeclaration of '%s'", yytext);
	sym = install(yytext, ns);
	next();
	return sym;
}

static struct dcldata *declarator0(struct dcldata *dp, uint8_t ns);

static struct dcldata *
directdcl(struct dcldata *dp, uint8_t ns)
{
	register Symbol *sym;

	if (accept('(')) {
		dp = declarator0(dp, ns);
		expect(')');
	} else {
		if (yytoken == IDEN || yytoken == TYPEIDEN)
			sym = newiden(ns);
		else
			sym = install("", ns);
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
declarator0(struct dcldata *dp, uint8_t ns)
{
	register uint8_t  n;

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
declarator(Type *tp, int8_t flags, uint8_t ns)
{
	struct dcldata data[NR_DECLARATORS+2];
	register struct dcldata *bp;
	Symbol *sym;

	memset(data, 0, sizeof(data));
	data[NR_DECLARATORS].op = 255;
	for (bp = declarator0(data, ns)-1; bp >= data; --bp) {
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
specifier(int8_t *sclass)
{
	Type *tp = NULL;
	int8_t qlf, sign, type, cls, size, t;

	qlf = sign = type = cls = size = 0;

	for (;;) {
		register int8_t *p;
		Type *(*dcl)(void) = NULL;

		switch (yytoken) {
		case SCLASS: p = &cls; break;
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
				dcl = enumdcl; p = &type; break;
			case STRUCT: case UNION:
				dcl = structdcl; p = &type; break;
			case VOID:   case BOOL:  case CHAR:
			case INT:    case FLOAT: case DOUBLE:
				p = &type; break;
			case SIGNED: case UNSIGNED:
				p = &sign; break;
			case LONG:
				if (size == LONG) {
					size = LONG+LONG;
					break;
				}
			case SHORT:
				p = &size; break;
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
	if (!sym->s.isdefined)
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
newtag(uint8_t tag)
{
	register Symbol *sym;
	static uint8_t ns = NS_STRUCTS;

	switch (yytoken) {
	case IDEN: case TYPEIDEN:
		if ((sym = lookup(yytext, NS_TAG)) == NULL)
			sym = install(yytext, NS_TAG);
		next();
		break;
	default:
		sym = install("", NS_TAG);
		break;
	}
	if (!sym->type) {
		if (ns == NS_STRUCTS + NR_MAXSTRUCTS)
			error("too much tags declared");
		sym->type = mktype(NULL, tag, 0, NULL);
		sym->type->ns = ns++;
	}
	
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
	uint8_t tag, n;
	size_t siz;

	tag = yylval.token;
	next();
	tagsym = newtag(tag);
	tagtype = tagsym->type;
	if (!accept('{'))
		return tagtype;

	if (tagtype->defined)
		error("redefinition of struct/union '%s'", yytext);
	tagtype->defined = 1;
	emitstruct(tagsym);

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
			sym->s.isfield = 1;
			tp = sym->type;
			if (tp->op == FTN)
				error("invalid type in struct/union");
			if (bp == &buff[NR_MAXSTRUCTS])
				error("too much fields in struct/union");
			*bp++ = sym->type;
			emitdcl(sym);
		} while (accept(','));
		expect(';');
	}

	emitestruct();
	n = bp - buff - 1;
	siz = sizeof(Type *) * n;
	tagtype->n.elem = n;
	tagtype->pars = memcpy(xmalloc(siz), buff, siz);
	return tagtype;
}

static Type *
enumdcl(void)
{
	register Type *tp;
	Symbol *sym;
	int val = 0;

	next();
	tp = newtag(ENUM)->type;

	if (yytoken == ';')
		return tp;

	expect('{');
	if (tp->defined)
		error("redefinition of enumeration '%s'", yytext);
	tp->defined = 1;
	while (yytoken != '}') {
		if (yytoken != IDEN)
			unexpected();
		sym = newiden(NS_IDEN);
		sym->type = inttype;
		if (accept('='))
			initializer(sym);
		sym->u.i = val++;
		if (!accept(','))
			break;
	}
	expect('}');

	return tp;
}

static Type *
parameter(void)
{
	Symbol *sym;
	int8_t sclass;
	Type *tp;

	if ((tp = specifier(&sclass)) == voidtype)
		return tp;
	sym = declarator(tp, ID_ACCEPTED, NS_IDEN);
	tp = sym->type;
	if (tp->op == FTN)
		error("incorrect function type for a function parameter");
	if (tp->op == ARY)
		tp = mktype(tp->type, PTR, 0, NULL);
	switch (sclass) {
	case REGISTER:
		sym->s.isregister = 1;
		break;
	case 0:
		sym->s.isauto = 1;
		break;
	default:
		error("bad storage class in function parameter");
	}
	return tp;
}

void
decl(void)
{
	Type *tp;
	Symbol *sym;
	int8_t sclass, isfun;

	tp = specifier(&sclass);
	if (accept(';'))
		return;

	do {
		sym = declarator(tp, ID_EXPECTED, NS_IDEN);
		isfun = sym->type->op == FTN;

		switch (sclass) {
		case TYPEDEF:
			sym->token = TYPEIDEN;
			continue;
		case STATIC:
			sym->s.isstatic = 1;
			break;
		case EXTERN:
			sym->s.isdefined = 0;
			break;
		case REGISTER:
			sym->s.isregister = 1;
			if (isfun)
				goto bad_function;
			break;
		case AUTO:
			if (isfun)
				goto bad_function;
			/* passtrough */
		default:
			sym->s.isauto = 1;
			break;
		}
		if (accept('='))
			initializer(sym);
		emitdcl(sym);
	} while (accept(','));

	expect(';');
	return;

bad_function:
	error("invalid storage class for function '%s'", sym->name);
}

Type *
typename(void)
{
	int8_t sclass;
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
	int8_t sclass;
	Symbol *sym;
	extern Symbol *curfun;

	switch (yytoken) {
	case IDEN: case TYPE: case TYPEIDEN: case SCLASS: case TQUALIFIER:
		base = specifier(&sclass);
		if (accept(';'))
			return;
		do {
			sym = declarator(base, ID_EXPECTED, NS_IDEN);
			tp = sym->type;
			sym->s.isstatic = 1;
			sym->s.isglobal= 1;

			switch (sclass) {
			case REGISTER: case AUTO:
				error("incorrect storage class for file-scope declaration");
			case STATIC:
				sym->s.isglobal = 0;
				break;
			case EXTERN:
				sym->s.isdefined = 0;
				break;
			case TYPEDEF:
				sym->token = TYPEIDEN;
				continue;
			}

			if (tp->op != FTN) {
				if (accept('='))
					initializer(sym);
				emitdcl(sym);
			} else if (yytoken == '{') {
				curfun = sym;
				emitfun(sym);
				context(NULL, NULL, NULL);
				emitefun();
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

