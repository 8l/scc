#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "code.h"
#include "tokens.h"
#include "symbol.h"


struct ctype *expr(void);

static struct ctype *
primary(void)
{
	register struct ctype *tp;

	switch (yytoken) {
	case IDEN:
		if (yylval.sym == NULL)
			error("'%s' undeclared", yytext);
		emitsym(yylval.sym);
		tp = yylval.sym->type;
		next();
		break;
	case CONSTANT:
		next();
		/* TODO: do something */
		break;
	case '(':
		next();
		tp = expr();
		expect(')');
		break;
	default:
		tp = NULL;
	}
	return tp;
}

struct ctype *
expr(void)
{
	register struct ctype *tp;

	do
		tp = primary();
	while (yytoken == ',');

	return tp;
}
