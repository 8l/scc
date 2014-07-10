#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sizes.h>
#include <cc.h>
#include "cc1.h"

#define ID_EXPECTED     1
#define ID_ACCEPTED     2

struct dcldata {
	uint8_t op;
	union {
		unsigned short nelem;
		Symbol *sym;
		struct funpars *pars;
		uint8_t qlf;
	} u;
};

static struct dcldata *declarator0(struct dcldata *dp, int8_t flags);

static struct dcldata *
arydcl(struct dcldata *dp)
{
	expect('[');
	expect(']');
	if (dp->op == 255)
		error("too much declarators");
	dp->u.nelem = 0;
	dp->op = ARY;
	return dp + 1;
}

static struct dcldata *
fundcl(struct dcldata *dp)
{
	expect('(');
	expect(')');;
	if (dp->op == 255)
		error("too much declarators");
	dp->op = FTN;
	dp->u.pars = NULL;
	return dp + 1;
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

static struct dcldata *
directdcl(struct dcldata *dp, int8_t flags)
{
	register Symbol *sym;

	if (accept('(')) {
		dp = declarator0(dp, flags);
		expect(')');
	} else if (flags) {
		if (yytoken != IDEN) {
			if (flags & ID_EXPECTED)
				goto expected;
			sym = install("", NS_IDEN);
		} else {
			sym = newiden();
		}
		dp->op = IDEN;
		dp->u.sym = sym;
		++dp;
	}

	for (;;) {
		switch (yytoken) {
		case '(':  dp = fundcl(dp); break;
		case '[':  dp = arydcl(dp); break;
		default:   return dp;
		}
	}

expected:
	error("expected '(' or identifier before of '%s'" , yytext);
}

static struct dcldata*
declarator0(struct dcldata *dp, int8_t flags)
{
	uint8_t buffer[NR_DECLARATORS];
	register uint8_t *bp, n, qlf;

	bp = buffer;
	for (n = 0; accept('*'); ++n) {
		if (n == NR_DECLARATORS)
			goto too_much_declarators;
		qlf = 0;
		if (yytoken == TQUALIFIER) {
			qlf |= yylval.token;
			next();
		}
		*bp++ = qlf;
	}

	dp = directdcl(dp, flags);

	bp = buffer;
	while (n--) {
		if (dp->op == 255)
			goto too_much_declarators;
		dp->op = PTR;
		dp->u.qlf = *bp++;
		++dp;
	}

	return dp;

too_much_declarators:
	error("too much declarators");
}

static Symbol *
declarator(Type *tp, int8_t flags)
{
	struct dcldata data[NR_DECLARATORS+1];
	register struct dcldata *bp;
	Symbol *sym = NULL;
	static Symbol dummy;

	memset(data, 0, sizeof(data));
	data[NR_DECLARATORS].op = 255;
	for (bp = declarator0(data, flags); bp >= data; --bp) {
		switch (bp->op) {
		case PTR:
			tp = qualifier(mktype(tp, PTR, 0), bp->u.qlf);
			break;
		case ARY:
			tp = mktype(tp, ARY, bp->u.nelem);
			break;
		case FTN:
			tp = mktype(tp, FTN, 0);
			break;
		case IDEN:
			sym = bp->u.sym;
			break;
		}
	}
	if (!tp->defined)
		error("declared variable '%s' of incomplete type", sym->name);
	if (!sym)
		sym = &dummy;
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
		case TYPE:
			switch (yylval.token) {
			case ENUM:
				dcl = enumdcl; p = &type; break;
			case STRUCT: case UNION:
				dcl = structdcl; p = &type; break;
			case TYPENAME:
				tp = yylval.sym->type;
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
			goto check_types;
		}
		if (*p)
			goto invalid_type;
		*p = yylval.token;
		if (dcl)
			tp = aggregate(dcl);
		else
			next();
	}

check_types:
	if (!type) {
		if (!sign && !size) {
			warn(options.implicit,
			     "type defaults to 'int' in declaration");
		}
		type = INT;
	}
	if (sign && type != INT && type != CHAR ||
	    size == SHORT && type != INT ||
	    size == LONG  && type != INT && type != DOUBLE ||
	    size == LONG+LONG && type != INT) {
		goto invalid_type;
	}
	if (sclass)
		*sclass = cls;
	if (!tp)
		tp = ctype(type, sign, size);
	return (qlf) ? qualifier(tp, qlf) : tp;

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
	for (q = p = tp->u.fields; p; q = p, p = p->next) {
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
		tp->u.fields = p;
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
	case IDEN: case TYPE: case TQUALIFIER:
		tp = specifier(NULL);
	case ';':
		break;
	default:
		error("declaration expected");
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

	if (yytoken == IDEN) {
		if ((sym = lookup(yytext, NS_TAG)) == NULL)
			sym = install(yytext, NS_TAG);
		next();
	} else {
		sym = install("", NS_TAG);
	}
	if (!(tp = sym->type))
		tp = sym->type = mktype(NULL, tag, 0);
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
			error("identifier expected");
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

void
decl(void)
{
	Type *tp;
	Symbol *sym;
	int8_t sclass;

	tp = specifier(&sclass);
	if (yytoken != ';') {
		do {
			sym = declarator(tp, ID_EXPECTED);
			switch (sclass) {
			case REGISTER: sym->s.isregister = 1; break;
			case STATIC: sym->s.isstatic = 1; break;
			case EXTERN: /* TODO: */; break;
			case TYPEDEF: /* TODO: */;break;
			case AUTO: default: sym->s.isauto = 1;
			}
			if (accept('='))
				initializer(sym);
			emitdcl(sym);
		} while (accept(','));
	}
	expect(';');
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
	sym = declarator(tp, 0);
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
	case IDEN: case TYPE: case SCLASS: case TQUALIFIER:
		base = specifier(&sclass);
		if (sclass == REGISTER || sclass == AUTO)
			error("incorrect storage class for file-scope declaration");
		if (accept(';'))
			return;
		do {
			sym = declarator(base, ID_EXPECTED);
			tp = sym->type;
			sym->s.isstatic = 1;

			if (sclass != STATIC)
				sym->s.isglobal = 1;
			else if (sclass != EXTERN)
				sym->s.isdefined = 1;

			if (BTYPE(tp) != FTN) {
				if (accept('='))
					initializer(sym);
				emitdcl(sym);
			} else if (yytoken == '{') {
				curfun = sym;
				emitfun(sym);
				emitsframe(sym);
				context(NULL, NULL, NULL);
				emiteframe();
				freesyms(NS_LABEL);
				return;
			}
		} while (accept(','));
		/* PASSTHROUGH */
	case ';':
		expect(';');
		return;
	case '@':
		next();
		emitprint(expr());
		expect(';');
		return;
	default:
		error("declaration expected");
	}
}

