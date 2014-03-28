#include <stdint.h>
#include <stdio.h>

#include "cc.h"

void expr(void);

static struct node *
primary(void)
{
	register struct node *np;

	switch (yytoken) {
	case IDEN:
		if (yylval.sym == NULL)
			error("'%s' undeclared", yytext);
		next();
		break;
	case CONSTANT:
		next();
		/* TODO: do something */
		break;
	case '(':
		next();
		expr();
		expect(')');
		break;
	default:
		np = NULL;
	}
	return np;
}


static void
ary(void)
{
}

static void
postfix(void)
{
	primary();
	for (;;) {
		switch (yytoken) {
		case '[': next(); ary(); break;
		default: return;
		}
	}			
}

void
expr(void)
{
	do
		postfix();
	while (yytoken == ',');
}
