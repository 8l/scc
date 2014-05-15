#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sizes.h>
#include <cc.h>
#include "cc1.h"

#define ID_EXPECTED     1
#define ID_ACCEPTED     2

int8_t forbid_eof;

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
	char *err;

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
		register uint8_t *p;
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

static struct node *
initializer(register Type *tp)
{
	if (accept('{')) {
		initializer(tp);
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
	static char *err;

	s = sym->name;
	for (q = p = tp->u.fields; p; q = p, p = p->next) {
		t = p->name;
		if (*s == *t && !strcmp(s, t))
			goto duplicated_name;
		if (sym->u.i == p->id)
			goto duplicated_value;
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

duplicated_name:
	err = "duplicated fields '%s' and '%s'";
	goto error;
duplicated_value:
	err = "duplicated enumeration fields '%s' and '%s'";
error:
	error(err, s, t);
}

static void
fielddcl(Type *base)
{
	Type *tp;
	Symbol *sym;
	char *err;

	switch (yytoken) {
	case SCLASS:
		goto bad_storage;
	case IDEN: case TYPE: case TQUALIFIER:
		tp = specifier(NULL);
	case ';':
		break;
	default:
		goto dcl_expected;
	}

	if (yytoken != ';') {
		do {
			sym = declarator(tp, ID_EXPECTED);
			newfield(base, sym);
		} while (accept(','));
	}

	expect(';');
	return;

bad_storage:
	err = "storage class '%s' in struct/union field";
	goto error;
dcl_expected:
	err = "declaration expected";
error:
	error(err, yytext);
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
		goto bad_tag;
	return tp;

bad_tag:
	error("'%s' defined as wrong kind of tag", yytext);
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
			goto redefined;
		tp->defined = 1;
		while (!accept('}'))
			fielddcl(tp);
	}

	return tp;

redefined:
	error("redefinition of struct/union '%s'", yytext);
}

static Type *
enumdcl(void)
{
	register Type *tp;
	Symbol *sym;
	int val = 0;
	char *err;

	next();
	tp = newtag(ENUM);
	if (yytoken != ';') {
		expect('{');
		if (tp->defined)
			goto redefined;
		tp->defined = 1;
		while (yytoken != '}') {
			if (yytoken != IDEN)
				goto iden_expected;
			sym = newiden();
			sym->type = inttype;
			if (accept('='))
				initializer(inttype);
			sym->u.i = val++;
			newfield(tp, sym);
			if (!accept(','))
				break;
		}
		expect('}');
	}

	return tp;

redefined:
	err = "redefinition of enumeration '%s'";
	goto error;
iden_expected:
	err = "identifier expected";
error:
	error(err, yytext);
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
				initializer(sym->type); /* TODO: emit initializer */
			emitdcl(sym);
		} while (accept(','));
	}
	expect(';');
}

Type *
typename(void)
{
	uint8_t sclass;
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
	Type *base;
	int8_t sclass;
	Symbol *sym;
	char *err;

	forbid_eof = 0; /* TODO: Fix when find EOF */

	switch (yytoken) {
	case IDEN: case TYPE: case SCLASS: case TQUALIFIER:
		base = specifier(&sclass);
		if (sclass == REGISTER || sclass == AUTO)
			goto bad_storage;
	case ';':
		break;
	case '@':
		next();
		emitprint(expr());
		goto semicolon;
	default:
		goto dcl_expected;
	}

	if (yytoken != ';') {
		do {
			Type *tp;

			sym = declarator(base, ID_EXPECTED);
			tp = sym->type;

			if (!(sclass & STATIC))
				sym->s.isglobal = 1;
			if (BTYPE(tp) == FTN) {
				if (yytoken == '{') {
					extern Symbol *curfun;

					curfun = sym;
					emitfun(sym);
					emitsframe(sym);
					context(compound, NULL, NULL, NULL);
					emiteframe(sym); /* FIX: sym is not used */
					freesyms(NS_LABEL);
					return;
				}
			} else {
				sym->s.isstatic = 1;
				if (sclass & EXTERN)
					; /* TODO: handle extern */
				else if (accept('='))
					initializer(tp);
				emitdcl(sym);

			}
		} while (accept(','));
	}

semicolon:
	expect(';');
	return;

bad_storage:
	err = "incorrect storage class for file-scope declaration";
	goto error;
dcl_expected:
	err = "declaration expected";
error:
	error(err);
}
