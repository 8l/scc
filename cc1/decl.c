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
newiden(void)
{
	Symbol *sym;
	extern uint8_t curctx;

	if (yylval.sym && yylval.sym->ctx == curctx)
		error("redeclaration of '%s'", yytext);
	sym = install(yytext, NS_IDEN);
	next();
	return sym;
}

static struct dcldata *declarator0(struct dcldata *dp);

static struct dcldata *
directdcl(struct dcldata *dp)
{
	register Symbol *sym;

	if (accept('(')) {
		dp = declarator0(dp);
		expect(')');
	} else {
		if (yytoken == IDEN || yytoken == TYPEIDEN)
			sym = newiden();
		else
			sym = install("", NS_IDEN);
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
declarator0(struct dcldata *dp)
{
	register uint8_t  n;

	for (n = 0; accept('*'); ++n) {
		while (accept(TQUALIFIER))
			/* nothing */;
	}

	dp = directdcl(dp);

	while (n--)
		dp = queue(dp, PTR, 0, NULL);

	return dp;
}

static Symbol *
declarator(Type *tp, int8_t flags)
{
	struct dcldata data[NR_DECLARATORS+2];
	register struct dcldata *bp;
	Symbol *sym;

	memset(data, 0, sizeof(data));
	data[NR_DECLARATORS].op = 255;
	for (bp = declarator0(data)-1; bp >= data; --bp) {
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
			tp = aggregate(dcl);
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
	register Type *tp = sym->type;

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
}

/* TODO: bitfields */

static void
newfield(Type *tp, Symbol *sym)
{
	register Field *p, *q;
	register char *s, *t;

	s = sym->name;
	for (q = p = tp->pars; p; q = p, p = p->next) {
		t = p->name;
		if (*s == *t && !strcmp(s, t))
			error("duplicated fields '%s' and '%s'", s, t);
		if (sym->u.i == p->id)
			error("duplicated enumeration fields '%s' and '%s'",
			      s, t);
	}

	p = xmalloc(sizeof(*p));
	p->name = xstrdup(s);
	p->next = NULL;
	p->id = sym->id;
	p->type = sym->type;
	if (!q)
		tp->pars= p;
	else
		q->next = p;

	return;
}

static void
fielddcl(Type *base)
{
	Type *tp;
	Symbol *sym;

	switch (yytoken) {
	case SCLASS:
		error("storage class '%s' in struct/union field", yytext);
	case IDEN: case TYPE: case TYPEIDEN: case TQUALIFIER:
		tp = specifier(NULL);
	case ';':
		break;
	default:
		unexpected();
	}

	if (yytoken != ';') {
		do {
			sym = declarator(tp, ID_EXPECTED);
			newfield(base, sym);
		} while (accept(','));
	}

	expect(';');
	return;
}

static Type *
newtag(uint8_t tag)
{
	register Symbol *sym;
	register Type *tp;

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
	if (!sym->type)
		sym->type = mktype(NULL, tag, 0, NULL);
	tp = sym->type;
	if (tp->op != tag)
		error("'%s' defined as wrong kind of tag", yytext);
	return tp;
}

static Type *
structdcl(void)
{
	Type *tp;
	uint8_t tag;

	tag = yylval.token;
	next();
	tp = newtag(tag);
	if (accept('{')) {
		if (tp->defined)
			error("redefinition of struct/union '%s'", yytext);
		tp->defined = 1;
		while (!accept('}'))
			fielddcl(tp);
	}

	return tp;
}

static Type *
enumdcl(void)
{
	register Type *tp;
	Symbol *sym;
	int val = 0;

	next();
	tp = newtag(ENUM);
	if (yytoken == ';')
		return tp;

	expect('{');
	if (tp->defined)
		error("redefinition of enumeration '%s'", yytext);
	tp->defined = 1;
	while (yytoken != '}') {
		if (yytoken != IDEN)
			unexpected();
		sym = newiden();
		sym->type = inttype;
		if (accept('='))
			initializer(sym);
		sym->u.i = val++;
		newfield(tp, sym);
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
	sym = declarator(tp, ID_ACCEPTED);
	sym->s.isdefined = 1;
	/* TODO: check type of the parameter */
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
	return sym->type;
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
		sym = declarator(tp, ID_EXPECTED);
		sym->s.isdefined = 1;
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
	sym = declarator(tp, ID_FORBIDDEN);
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
			sym = declarator(base, ID_EXPECTED);
			tp = sym->type;
			sym->s.isstatic = 1;
			sym->s.isglobal= 1;
			sym->s.isdefined = 1;

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

