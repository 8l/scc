#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "syntax.h"
#include "symbol.h"

int8_t forbid_eof;

static struct dcldata
	*declarator0(struct dcldata *dp, uint8_t ns, int8_t flags);

struct dcldata {
	uint8_t op;
	union {
		unsigned short nelem;
		struct symbol *sym;
		struct funpars *pars;
		uint8_t qlf;
	} u;
};

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

static struct dcldata*
directdcl(struct dcldata *dp, uint8_t ns, int8_t flags)
{
	register struct symbol *sym;
	char *err;

	if (accept('(')) {
		dp = declarator0(dp, ns, flags);
		expect(')');
	} else if (flags) {
		if (yytoken != IDEN) {
			if (flags > 0)
				goto expected;
			sym = install(NULL, ns);
		} else {
			sym = lookup(yytext, ns);
			if (sym && sym->ctx == curctx)
				goto redeclared;
			sym = install(yytext, ns);
			next();
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

redeclared:
	err = "redeclaration of '%s'";
	goto error;
expected:
	err = "expected '(' or identifier before of '%s'";
error:	error(err, yytext);
}

static struct dcldata*
declarator0(struct dcldata *dp, uint8_t ns, int8_t flags)
{
	if (accept('*')) {
		register uint8_t qlf = 0;
		while (yytoken == TQUALIFIER) {
			qlf |= yylval.sym->u.c;
			next();
		}
		dp = declarator0(dp, ns, flags);
		if (dp->op == 255)
			error("too much declarators");
		dp->op = PTR;
		dp->u.qlf = qlf;
		return dp + 1;
	} else {
		return directdcl(dp, ns, TQUALIFIER);
	}
}

static struct symbol *
declarator(struct ctype *tp, uint8_t ns, int8_t flags)
{
	struct dcldata data[NR_DECLARATORS+1];
	register struct dcldata *bp;
	struct symbol *sym;

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
	sym->type = tp;
	return sym;
}

static struct ctype *structdcl(void), *enumdcl(void);

static struct ctype *
specifier(int8_t *sclass)
{
	struct ctype *tp = NULL;
	int8_t qlf, sign, type, cls, cplex, size;

	qlf = sign = type = cls = size = cplex = 0;

	for (;;) {
		register uint8_t *p;
		struct symbol *sym = yylval.sym;

		switch (yytoken) {
		case SCLASS: p = &cls; break;
		case TQUALIFIER:
			if ((qlf |= sym->u.c) & RESTRICT)
				goto invalid_type;
			goto next_token;
		case TYPE:
			switch (sym->u.c) {
			case ENUM:
				tp = enumdcl();   goto set_type;
			case STRUCT:   case UNION:
				tp = structdcl(); goto set_type;
			case TYPENAME:
				tp = yylval.sym->type;
			case VOID:   case BOOL:  case CHAR:
			case INT:    case FLOAT: case DOUBLE:
set_type:			p = &type; break;
			case SIGNED: case UNSIGNED:
				p = &sign; break;
			case LONG:
				if (size == LONG) {
					size = LONG+LONG;
					goto next_token;
				}
			case SHORT:
				p = &size; break;
			case COMPLEX: case IMAGINARY:
				p = &cplex; break;
			}
			break;
		default:
			goto check_types;
		}
		if (*p)
			goto invalid_type;
		*p = sym->u.c;
next_token:	next();
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
	    cplex && type != FLOAT && type != DOUBLE ||
	    size == SHORT && type != INT ||
	    size == LONG  && type != INT && type != DOUBLE ||
	    size == LONG+LONG && type != INT) {
		goto invalid_type;
	}
	if (sclass)
		*sclass = cls;
	if (!tp)
		tp = ctype(type, sign, size, cplex);
	return (qlf) ? qualifier(tp, qlf) : tp;

invalid_type:
	error("invalid type specification");
}

static struct node *
initializer(register struct ctype *tp)
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

static struct ctype *
structdcl(void)
{
	uint8_t type = yylval.sym->u.c;

	next();
	if (yytoken == IDEN) {
	}
	return NULL;
}

static struct ctype *
enumdcl(void)
{
	return NULL;
}

struct node *
decl(void)
{
	struct ctype *tp;
	struct symbol *sym;
	int8_t sclass;

	tp = specifier(&sclass);
	if (yytoken != ';') {
		do {
			 sym = declarator(tp, NS_IDEN, 1);
			/* assign storage class */
			if (accept('='))
				initializer(sym->type);
		} while (accept(','));
	}

	expect(';');
	return NULL;
}

static struct symbol *
fielddcl(void)
{
	struct ctype *tp;
	struct symbol *sym;

	switch (yytoken) {
	case IDEN:
		warn(options.implicit,
		     "type defaults to 'int' in declaration");
		tp = inttype;
		break;
	case SCLASS:
		error("storage class '%s' in struct/union field", yytext);
	case TYPE: case TQUALIFIER:
		tp = specifier(NULL);
		break;
	case ';':
		break;
	default:
		error("declaration expected");
	}

	if (yytoken != ';') {
		do {
			sym = declarator(tp, 0, 1);
			if (accept(':'))
				; /* TODO: bitfields */
			/* TODO: add to the aggregate */
		} while (accept(','));
	}

	expect(';');
	return NULL;
}

struct node *
typename(void)
{
	declarator(specifier(NULL), NS_IDEN, -1)->type;
	return  NULL;
}

struct node *
extdecl(void)
{
	struct ctype *tp;
	int8_t sclass;
	struct symbol *sym;
	extern struct symbol *curfun;

	forbid_eof = 1;

	switch (yytoken) {
	case IDEN:
		warn(options.implicit,
		     "type defaults to 'int' in declaration");
		tp = inttype;
		break;
	case TYPE: case SCLASS: case TQUALIFIER:
		tp = specifier(&sclass);
		if (sclass == REGISTER || sclass == AUTO)
			error("incorrect storage class for file-scope declaration");
		break;
	case ';':
		break;
	default:
		error("declaration expected");
	}

	if (yytoken != ';') {
		do {
			extern void printtype(struct ctype *tp);
			sym = declarator(tp, NS_IDEN, 1);
			printtype(sym->type);
			/* assign storage class */
			if (isfun(sym)) {
				if (yytoken == '{') {
					curfun = sym;
					context(function);
					freesyms(NS_LABEL);
				}
			} else if (accept('=')) {
				initializer(sym->type);
			}
		} while (accept(','));
	}

	forbid_eof = 0;
	expect(';');
	return NULL;
}
