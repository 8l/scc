
#include <stdio.h>

#include "symbol.h"
#include "tokens.h"
#include "syntax.h"

void stmt(void);

static void
do_goto(void)
{
	expect(GOTO);
	expect(IDEN);
}

static void
do_while(void)
{
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
	stmt();
}

static void
do_do(void)
{
	expect(DO);
	stmt();
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
}

static void
do_for(void)
{
	expect(FOR);
	expect('(');
	if (yytoken != ';')
		expr();
	expect(';');
	if (yytoken != ';')
		expr();
	expect(';');
	if (yytoken != ')')
		expr();
	expect(')');
	stmt();
}

static void
do_if(void)
{
	expect(IF);
	expect('(');
	expr();
	expect(')');
	stmt();
	if (accept(ELSE))
		stmt();

}

static void
do_switch(void)
{
	expect(SWITCH);
	expect('(');
	expr();
	expect(')');
	stmt();
}

void
stmt(void)
{

	switch (yytoken) {
	case '{':
		compound();
		break;
	case SWITCH:
		do_switch();
		break;
	case IF:
		do_if();
		break;
	case FOR:
		do_for();
		break;
	case DO:
		do_do();
		break;
	case WHILE:
		do_while();
		break;
	case CONTINUE:
	case BREAK:
	case RETURN:
	case GOTO:
		do_goto();
		break;
	case CASE:
		/* TODO */
	case DEFAULT:
		/* TODO */
		break;
	case IDEN:
		/* TODO: check if it can be a label */
	default:
		prtree(expr());
		putchar('\n');
		break;
	}
	expect(';');
}

void
compound(void)
{
	expect('{');
	new_ctx();
	while (decl())
		/* nothing */;
	while (!accept('}'))
		stmt();
	del_ctx();
}
