#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "cc.h"
#include "tokens.h"
#include "types.h"
#include "syntax.h"
#include "symbol.h"

char parser_out_home;

#include <stdio.h>	/* TODO: remove this */

static void declarator(void);


static void dirdcl(void)
{
	puts("dirdecl");
	if (accept('(')) {
		declarator();
		expect(')');
	} else if (yytoken == IDENTIFIER) {
		if (yyval.sym && yyval.sym->level == nested_level)
			error("redeclaration of '%s'", yytext);
		addsym(yytext, yyhash);
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
			puts("leaving dirdcl");
			return;
		}
	}
}





/*
 *
 */

static struct type *types[][2] = {{T_VOID, NULL},
				  {T_SCHAR, T_UCHAR},
				  {T_SHORT, T_USHORT},
				  {T_INT, T_UINT},
				  {T_LONG, T_ULONG},
				  {T_LLONG, T_ULLONG},
				  {T_FLOAT, NULL},
				  {T_DOUBLE, NULL},
				  {T_LDOUBLE, NULL}};

#define F_VOID    0
#define F_CHAR    1
#define F_SHORT   2
#define F_INT     3
#define F_LONG    4
#define F_LLONG   5
#define F_FLOAT   6
#define F_DOUBLE  7
#define F_LDOUBLE 8

static struct type *specifier(void)
{
	auto unsigned char sign, sclass, tqlf, nt;
	auto struct type *t;

	puts("especifier");
	t = NULL;
	tqlf = sign = sclass = 0;
	for (;; next()) {
		switch (yytoken) {
		case TYPEDEF:case EXTERN:case STATIC:case AUTO:case REGISTER:
			if (sclass != 0)
				error("Two or more storage specifier");
			sclass = yytoken;
			continue;
		case CONST:
			if (!(tqlf ^= T_CONST))
				goto duplicated_qlf;
			continue;
		case RESTRICTED:
			if (!(tqlf ^= T_RESTRICTED))
				goto duplicated_qlf;
			continue;
		case VOLATILE:
			if (!(tqlf ^= T_VOLATILE))
				goto duplicated_qlf;
			continue;
		case VOID:   nt = F_VOID;   goto check_type;
		case CHAR:   nt = F_CHAR;   goto check_type;
		case SHORT:  nt = F_SHORT;  goto check_type;
		case INT:    nt = F_INT;    goto check_type;
		case FLOAT:  nt = F_FLOAT;  goto check_type;
		case DOUBLE: nt = F_DOUBLE; goto check_type;
		case LONG:   nt = F_LONG;   goto check_type;
		case SIGNED: case UNSIGNED:
			if (sign != 0) {
				error((sign != yytoken) ?
				      "signed and unsigned in declaration" :
				      "duplicated %s", yytext);
			}
			sign = yytoken;
			if (t == NULL)
				continue;     /* we don't have type now */
			goto check_type;
		case STRUCT:	/* TODO */
		case UNION:	/* TODO */
		case ENUM:	/* TODO */
		default:
			if (t) {
				return t;
			} else if (tqlf) {
				if (tqlf & T_CONST)	  pushtype(CONST);
				if (tqlf & T_RESTRICTED)  pushtype(RESTRICTED);
				if (tqlf & T_VOLATILE)	  pushtype(VOLATILE);
				return decl_type(t);
			} else if (nested_level == 0 && yytoken == IDENTIFIER) {
				warning_error(user_opt.implicit_int,
					      "type defaults to 'int' "
					      "in declaration of '%s'",
					      yytext);
				return T_INT;
			} else if (nested_level == 0) {
				error("declaration expected");
			}
			return NULL;
		}
	check_type:
		if (nt == F_LONG) {
			if (t == NULL ||
			    t == T_INT || t == T_UINT || t == T_LONG) {
				/* nothing */;
			} else if (t == T_LONG || t == T_ULONG) {
				nt = F_LLONG;
			} else if (t == T_DOUBLE) {
				nt = F_LDOUBLE;
			} else if (t == T_LLONG || t == T_ULLONG) {
				error("'long long long' is too long");
			} else if (t == T_LDOUBLE) {
				error("'long long double' is too long");
			} else {
				goto two_or_more_btype;
			}
		} else if (t != NULL) {
			goto two_or_more_btype;
		} if (nt == F_VOID && sign != 0) {
			goto incorrect_sign;
		} if (nt == F_CHAR && sign == 0) {
			t = T_UCHAR;	    /* char by default is unsigned */
		} else if (!(t = types[nt][sign == UNSIGNED])) {
			goto incorrect_sign;
		}
	}
duplicated_qlf:
	error("duplicated '%s'", yytext);
two_or_more_btype:
	error("two or more basic types");
incorrect_sign:
	error("sign specifier applied to incorrect type");
}


#undef F_VOID
#undef F_CHAR
#undef F_SHORT
#undef F_INT
#undef F_LONG
#undef F_LLong
#undef F_FLOAT
#undef F_DOUBLE
#undef F_LDOUBLE


static void declarator(void)
{
	unsigned char qlf[PTRLEVEL_MAX], *bp, *lim;

	puts("declarator");
	lim = qlf + PTRLEVEL_MAX;
	for (bp = qlf; yytoken == '*' && bp != lim; ++bp) {
		*bp = 0;
	repeat_qlf:
		switch (next()) {
		case CONST:
			if (!(*bp ^= T_CONST))
				goto duplicated;
			goto repeat_qlf;
		case RESTRICTED:
			if (!(*bp ^= T_RESTRICTED))
				goto duplicated;
			goto repeat_qlf;
		case VOLATILE:
			if (!(*bp ^= T_VOLATILE))
				goto duplicated;
			goto repeat_qlf;
		default:
			break;
		}
	}

	if (bp == lim)
		error("Too much indirection levels");

	dirdcl();

	while (bp-- != qlf) {
		if (*bp & T_CONST)	 pushtype(CONST);
		if (*bp & T_RESTRICTED)  pushtype(RESTRICTED);
		if (*bp & T_VOLATILE)	 pushtype(VOLATILE);
		pushtype(PTR);
	}

	puts("leaving dcl");
	return;

duplicated:
	error("duplicated '%s'", yytext);
}

unsigned char decl(void)
{
	auto struct type *tp, *tbase;
	auto unsigned char nd = 0;

	puts("decl");
	if (!(tbase = specifier()))
		return 0;
	if (yytoken != ';') {
		do {
			declarator();
			tp = decl_type(tbase);
			if (isfunction(tp) && yytoken == '{') {
				compound();
				goto leaving;
			}
			++nd;
		} while (accept(','));
	}

	expect(';');
	if (nd == 0) {
		warning_error(user_opt.useless_typename,
			      "useless type name in empty declaration");
	}
leaving:
	puts("leaving decl");
	return 1;
}

void type_name()
{
}
