#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "tokens.h"
#include "syntax.h"
#include "symbol.h"

char parser_out_home;
struct ctype *curfun;

static struct symbol *cursym;
static void declarator(void);

static void
newiden(void)
{
	switch (yyval.sym->ns) {
	case NS_ANY:        /* First aparrence of the symbol */
		yyval.sym->ns = NS_IDEN;
		cursym = yyval.sym;
		break;
	case NS_IDEN:
		if (yyval.sym->ctx == curctx)
			error("redeclaration of '%s'", yytext);
	default:
		cursym = lookup(yytext, NS_IDEN);
	}
}

static void
dirdcl(void)
{
	if (accept('(')) {
		declarator();
		expect(')');
	} else if (yytoken == IDEN) {
		newiden();
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

static unsigned char
spec(register struct ctype *cp)
{
	register unsigned char sign, n;

	for (n = sign = 0; ; ++n, next()) {
		switch (yytoken) {
		case TYPEDEF:  case EXTERN: case STATIC: case AUTO:
		case REGISTER: case CONST:  case VOLATILE:
			storage(cp, yytoken);
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
		case VOID:   case CHAR:   case SHORT:  case INT:
		case FLOAT:  case DOUBLE: case LONG:   case BOOL:
			cp->type = btype(cp->type, yytoken);
			break;
		case STRUCT:    /* TODO */
		case UNION:	/* TODO */
		case ENUM:	/* TODO */
		default:
			if (!cp->type && sign)
				cp->type = INT;
			return n;
		}
	}
duplicated:
	error("duplicated '%s'", yytext);
signed_and_unsigned:
	error("both 'signed' and 'unsigned' in declaration specifiers");
}

static void
declarator(void)
{
	unsigned char qlf[NR_DECLARATORS];
	register unsigned char *bp, *lim;

	lim = &qlf[NR_DECLARATORS];
	for (bp = qlf; yytoken == '*' && bp != lim; ) {
		next();
		*bp++ = PTR;
		while (bp != lim) {
			switch (yytoken) {
			case CONST: case VOLATILE: case RESTRICT:
				*bp++ = yytoken;
				next();
				break;
			default:
				goto continue_outer;
			}
		}
	continue_outer: ;
	}
	if (bp == lim)
		error("Too much type declarators");

	dirdcl();

	for (lim = bp - 1, bp = qlf; bp < lim; ++bp)
		pushtype(*bp);
}

static void
listdcl(register struct ctype *tp)
{
	do {
		declarator();
		curfun = decl_type(tp);
		if (!curfun->type) {
			warning_error(options.implicit,
				      "type defaults to 'int' in declaration of '%s'",
				      yytext);
		} else if (curfun->type == FTN && yytoken == '{') {
			function(cursym);
			return;
		}
	} while (accept(','));
	expect(';');
}

unsigned char
decl(void)
{
	register struct ctype *tp;

	if (accept(';'))
		return 1;

	tp = newctype();
	if (!spec(tp)) {
		if (curctx != CTX_OUTER)
			return 0;
		warning("data definition has no type or storage class");
	}
	if (accept(';')) {
		warning_error(options.useless,
			      "useless type name in empty declaration");
	} else {
		listdcl(tp);
	}
	delctype(tp);

	return 1;
}

void
type_name()
{

}
