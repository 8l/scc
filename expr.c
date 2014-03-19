#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "syntax.h"


struct node *expr(void);

static struct node *
primary(void)
{
	register struct node *np;
	register struct symbol *sym;

	switch (yytoken) {
	case IDEN:
		if (yylval.sym == NULL)
			error("'%s' undeclared", yytext);
		/* TODO: Do something */
		next();
		break;
	case CONSTANT:
		next();
		/* TODO: do something */
		break;
	case '(':
		next();
		np = expr();
		expect(')');
		break;
	default:
		np = NULL;
		break;
	}
	return np;
}

struct node *
expr(void)
{
	register struct node *np;

	do
		np = primary();
	while (yytoken == ',');

	return np;
}
