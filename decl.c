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

struct dcldata {
	uint8_t op;
	union {
		unsigned short nelem;
		struct symbol *sym;
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
	uint8_t buffer[NR_DECLARATORS];
	register uint8_t *bp, n, qlf;

	bp = buffer;
	for (n = 0; accept('*'); ++n) {
		if (n == NR_DECLARATORS)
			goto too_much_declarators;
		qlf = 0;
		if (yytoken == TQUALIFIER) {
			qlf |= yylval.sym->u.c;
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

static struct ctype *structdcl(uint8_t tag), *enumdcl(uint8_t tag);

static struct ctype *
specifier(int8_t *sclass)
{
	struct ctype *tp = NULL;
	int8_t qlf, sign, type, cls, cplex, size, t;

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
			case ENUM: case STRUCT: case UNION:
				t = sym->u.c;
				next();
				tp = (t == UNION) ? enumdcl(t) : structdcl(t);
				p = &type;
				goto check_spec;
			case TYPENAME:
				tp = yylval.sym->type;
			case VOID:   case BOOL:  case CHAR:
			case INT:    case FLOAT: case DOUBLE:
				p = &type; goto next_token;
			case SIGNED: case UNSIGNED:
				p = &sign; break;
			case LONG:
				if (size == LONG) {
					size = LONG+LONG;
					goto next_token;
				}
			case SHORT:
				p = &size; goto next_token;
			case COMPLEX: case IMAGINARY:
				p = &cplex; goto next_token;
			}
			break;
		default:
			goto check_types;
		}
next_token:	next();
check_spec:	if (*p)
			goto invalid_type;
		*p = sym->u.c;
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

/* TODO: add define for the 3 parameter of declarator */
/* TODO: bitfields */
static short
fielddcl(uint8_t ns, uint8_t type)
{
	struct ctype *tp;
	struct symbol *sym;
	short offset = 0, size;

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
			sym = declarator(tp, ns, 1);
			sym->u.offset = offset;
			size = sym->type->size;
			if (type == STRUCT)
				offset += size;
			else if (offset < size)
				offset = size;
		} while (accept(','));
	}

	expect(';');
	return offset;
}

static struct ctype *
newtag(uint8_t tag)
{
	register struct symbol *sym;
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
		sym = install(NULL, NS_TAG);
	}
	++namespace;
	return sym->type = mktype(NULL, tag, NULL, 0);

bad_tag:
	error("'%s' defined as wrong kind of tag", yytext);
}

static struct ctype *
structdcl(uint8_t tag)
{
	struct ctype *tp;
	short size = 0;

	tp = newtag(tag);
	if (yytoken != ';') {
		expect('{');
		if (tp->defined)
			goto redefined;
		while (!accept('}'))
			size = fielddcl(namespace, tag);
		tp->size = size;
		tp->defined = 1;
	}

	return tp;

redefined:
	error("redefinition of struct/union '%s'", yytext);
}

static struct ctype *
enumdcl(uint8_t token)
{
	/* TODO: create function for creating identifiers */
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
