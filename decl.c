#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"

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

static struct dcldata
	*declarator0(struct dcldata *dp, uint8_t ns, int8_t flags);

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
newiden(uint8_t ns)
{
	Symbol *sym;
	extern uint8_t curctx;

	if (yylval.sym && yylval.sym->ctx == curctx)
		error("redeclaration of '%s'", yytext);
	sym = install(yytext, ns);
	next();
	return sym;
}

static struct dcldata *
directdcl(struct dcldata *dp, uint8_t ns, int8_t flags)
{
	register Symbol *sym;
	char *err;

	if (accept('(')) {
		dp = declarator0(dp, ns, flags);
		expect(')');
	} else if (flags) {
		if (yytoken != IDEN) {
			if (flags & ID_EXPECTED)
				goto expected;
			sym = install("", ns);
		} else {
			sym = newiden(ns);
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
declarator0(struct dcldata *dp, uint8_t ns, int8_t flags)
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

	dp = directdcl(dp, ns, flags);

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
declarator(Type *tp, uint8_t ns, int8_t flags)
{
	struct dcldata data[NR_DECLARATORS+1];
	register struct dcldata *bp;
	Symbol *sym = NULL;
	static Symbol dummy;

	memset(data, 0, sizeof(data));
	data[NR_DECLARATORS].op = 255;
	for (bp = declarator0(data, ns, flags); bp >= data; --bp) {
		switch (bp->op) {
		case PTR:
			tp = qualifier(mktype(tp, PTR, NULL, 0), bp->u.qlf);
			break;
		case ARY:
			tp = mktype(tp, ARY, NULL, bp->u.nelem);
			break;
		case FTN:
			tp = mktype(tp, FTN, NULL, 0);
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
			tp = dcl();
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
	register struct field *p, *q;
	register char *s, *t;
	static uint8_t op;
	static char *err;

	s = sym->name;
	op = tp->op;
	for (q = p = tp->u.fields; p; q = p, p = p->next) {
		t = p->sym->name;
		if (*s == *t && !strcmp(s, t))
			goto duplicated_name;
		if (op == ENUM && sym->u.i == p->sym->u.i)
			goto duplicated_value;
	}

	p = xmalloc(sizeof(*p));
	p->next = NULL;
	p->sym = sym;
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
fielddcl(Type *base, uint8_t ns)
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
			sym = declarator(tp, ns, ID_EXPECTED);
			newfield(tp, sym);
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
	Type *tp;
	extern uint8_t namespace;

	if (yytoken == IDEN) {
		sym = lookup(yytext, NS_TAG);
		if (sym) {
			if (sym->type->op != tag)
				goto bad_tag;
		} else {
			sym = install(yytext, NS_TAG);
		}
		next();
	} else {
		sym = install("", NS_TAG);
	}
	tp = sym->type = mktype(NULL, tag, NULL, 0);
	sym->u.ns = ++namespace;
	tp->sym = sym;
	return tp;

bad_tag:
	error("'%s' defined as wrong kind of tag", yytext);
}

static Type *
structdcl(void)
{
	Type *tp;
	uint8_t ns, tag;

	tag = yylval.token;
	next();
	tp = newtag(tag);
	tp->u.fields = NULL;
	ns = tp->sym->u.ns;
	if (accept('{')) {
		if (tp->defined)
			goto redefined;
		tp->defined = 1;
		while (!accept('}'))
			 fielddcl(tp, ns);
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
			sym = newiden(NS_IDEN);
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
			 sym = declarator(tp, NS_IDEN, ID_EXPECTED);
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
	sym = declarator(tp, NS_IDEN, 0);
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

			sym = declarator(base, NS_IDEN, ID_EXPECTED);
			tp = sym->type;

			if (!(sclass & STATIC))
				sym->s.isglobal = 1;
			if (BTYPE(tp) == FTN) {
				emitfun(sym);
				if (yytoken == '{') {
					extern Symbol *curfun;

					curfun = sym;
					emitsframe(sym);
					context(compound);
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
