#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "syntax.h"
#include "symbol.h"

char parser_out_home;


static void declarator(void);

static struct symbol *newiden(char *s, unsigned char key)
{
	register struct symbol *sym = lookup(yytext, yyhash);

	if (!sym)
		sym = install(yytext, yyhash);
	else if (sym->level == nested_level)
		error("redeclaration of '%s'", yytext);
	return sym;
}

static void dirdcl(void)
{
	if (accept('(')) {
		declarator();
		expect(')');
	} else if (yytoken == IDEN) {
		newiden(yytext, yyhash);
		next();
	} else {
		error("expected '(' or identifier before of '%s'", yytext);
	}

	for (;;) {
		if (accept('(')) {
			next();
			pushtype(FTN);
			if (accept(')'))
				; /* TODO: k&r function */
			else
				/* TODO: prototyped function */;
			continue;
		} else if (accept('[')) {
			pushtype(ARY);
			if (accept(']'))
				; /* TODO: automatic size array */
			else
				/* TODO: specify size of array */;
			continue;
		} else {
			return;
		}
	}
}

static unsigned char spec(register struct ctype *cp)
{
	register unsigned char sign, n;

	for (n = sign = 0; ; ++n, next()) {
		switch (yytoken) {
		case TYPEDEF:  case EXTERN: case STATIC: case AUTO:
		case REGISTER: case CONST:  case VOLATILE:
			ctype(cp, yytoken);
			break;
		case UNSIGNED:
			cp->c_unsigned = 1;
		case SIGNED:
			if (sign == yytoken)
				goto duplicated;
			if (sign)
				goto signed_and_unsigned;
			sign = yytoken;
			break;
		case VOID:   case CHAR: case SHORT:  case INT: case FLOAT:
		case DOUBLE: case LONG: case BOOL:
			cp->base = btype(cp->base, yytoken);
			break;
		case STRUCT:    /* TODO */
		case UNION:	/* TODO */
		case ENUM:	/* TODO */
		default:
			if (!cp->base && sign)
				cp->base = T_INT;
			return n;
		}
	}
duplicated:
	error("duplicated '%s'", yytext);
signed_and_unsigned:
	error("both 'signed' and 'unsigned' in declaration specifiers");
}

static void declarator(void)
{
	unsigned char qlf[NR_DECLARATORS];
	register unsigned char *bp, *lim;

	lim = &qlf[NR_DECLARATORS];
	for (bp = qlf; yytoken == '*' && bp != lim; ++bp) {
		*bp++ = PTR;
		for (;;) {
			next();
			switch (yytoken) {
			case CONST: case VOLATILE: case RESTRICT:
				*bp++ = yytoken;
				break;
			default:
				goto next_pointer;
			}
		}
	next_pointer: ;
	}
	if (bp == lim)
		error("Too much type declarators");

	dirdcl();

	for (lim = bp - 1, bp = qlf; bp < lim; ++bp)
		pushtype(*bp);
}

static void listdcl(register struct ctype *cp)
{
	register  struct type *tp;

	do {
		declarator();
		tp = decl_type(cp->base);
		if (!tp) {
			warning_error(user_opt.implicit_int,
				      "type defaults to 'int' in declaration of '%s'",
				      yytext);
		} else if (tp->op == FTN && yytoken == '{') {
			compound();
			return;
		}
	} while (accept(','));
}

unsigned char decl(void)
{
	static struct ctype ctype;

	if (accept(';'))
		return 1;
	if (!spec(&ctype)) {
		if (nested_level != 0)
			return 0;
		warning("data definition has no type or storage class");
	}
	if (yytoken == ';') {
		warning_error(user_opt.useless_typename,
			      "useless type name in empty declaration");
	} else {
		listdcl(&ctype);
	}
	expect(';');

	return 1;
}

void type_name()
{
}
