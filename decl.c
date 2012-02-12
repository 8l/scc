#include <alloca.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "types.h"


/* ANSI C says minimum maximum for indirection level is 12 */
#define PTRLEVEL_MAX 12

char parser_out_home;

static unsigned char symhash;
static char symname[TOKSIZ_MAX + 1];
static unsigned char stack[30];
static unsigned char *stackp = stack;

#define push(x) (*stackp++ = (x))
#define pop()   (*--stackp)
#define empty() (stackp == stack)


#include <stdio.h>	/* TODO: remove this */

void decl(void);


void dirdcl(void)
{
	puts("dirdecl");
	if (yytoken == '(') {
		gettok();
		decl();
		if (yytoken != ')')
			error("expected ')'");
		gettok();
	} else if (yytoken == IDENTIFIER) {
		gettok();
		strcpy(symname, yytext);
		symhash = yyhash;
	} else {
		error("expected '(' or identifier before of '%s'", yytext);
	}

	for (;;) {
		switch (yytoken) {
		case '(':
			push(FTN);
			if (gettok() == ')')
				gettok();
			else
				/* TODO: prototyped function */;
			continue;
		case '[':
			push(ARY);
			if (gettok() == ']')
				gettok();
			else
				/* TODO: specify size of array */;
			continue;
		default:
			printf("leaving dirdcl %c\n", yytoken);
			return;
		}
	}
}





/*
 *
 */

struct type *types[][2] = {{T_VOID, NULL},
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

struct type *specifier(void)
{
	static char sign, sclass, tqlf, nt;
	struct type *t;

repeat:
	t = NULL;
	tqlf = sign = sclass = 0;
	for (;;) {
		switch (gettok()) {
		case TYPEDEF:case EXTERN:case STATIC:case AUTO:case REGISTER:
			if (sclass != 0)
				error("Two or more storage specifier");
			sclass = yytoken;
			continue;
		case CONST: case VOLATILE: case RESTRICTED:
			/* TODO */
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
		case ';':
			if (t != NULL) {
				warning("useless type name in empty "
					"declaration");
			}
			goto repeat;
		case IDENTIFIER:
			/* TODO: deal with user types */
			if (t == NULL) {
				warning_error(user_opt.implicit_int,
					      "type defaults to 'int' "
					      "in declaration of", yytext);
				return T_INT;
			}
		default:
			return t;
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


void decl(void)
{
	unsigned char ns = 0;
	unsigned char qlf[PTRLEVEL_MAX];

	puts("decl");
	for (ns = 0; yytoken == '*'; ns++) {
		if (ns == PTRLEVEL_MAX)
			error("Too much indirection levels");
		switch (gettok()) {
		case CONST:
			if (!(qlf[ns] ^= 2))
				goto duplicated;
			continue;
		case RESTRICTED:
			if (!(qlf[ns] ^= 4))
				goto duplicated;
			continue;
		case VOLATILE:
			if (!(qlf[ns] ^= 8))
				goto duplicated;
			continue;
		}
	}
	dirdcl();

	if (ns)
		push(PTR);	/* TODO: pointer qualifiers */
	printf("leaving dcl %c\n", yytoken);
	return;

duplicated:
	error("duplicated '%s'", yytext);
}



void declaration(void)
{
	struct type *t;
	struct symbol *sym;

repeat:
	t = specifier();

	for (; ; gettok()) {
		/* TODO: put here savepoint for error recovering */
		decl();
		if (yytoken != ',' && yytoken != ';')
			error("unexpected", yytext);
		while (!empty())
			t = mktype(t, pop());
		ptype(t);
		sym = alloca(sizeof(*sym));
		pushsym(siden, sym, symhash);

		if (yytoken == ',')
			continue;
		else if (yytoken == ';')
			goto repeat;
	}
}




#include <stddef.h>


int main(int argc, char *argv[])
{
	init_lex();

	open_file(NULL);
	declaration();

	return 0;
}
