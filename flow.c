
#include <stdio.h>

#include "symbol.h"
#include "tokens.h"
#include "syntax.h"

static struct node *stmt(void);

static struct node *
do_goto(void)
{
	expect(GOTO);
	expect(IDEN);
	return NULL;
}

static struct node *
do_while(void)
{
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
	stmt();
	return NULL;
}

static struct node *
do_do(void)
{
	expect(DO);
	stmt();
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
	return NULL;
}

static struct node *
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
	return NULL;
}

static struct node *
do_if(void)
{
	expect(IF);
	expect('(');
	expr();
	expect(')');
	stmt();
	if (accept(ELSE))
		stmt();
	return NULL;

}

static struct node *
do_switch(void)
{
	expect(SWITCH);
	expect('(');
	expr();
	expect(')');
	stmt();
	return NULL;
}

static struct node *
stmt(void)
{
	register struct node *np;

	switch (yytoken) {
	case '{':      np = compound();  break;
	case SWITCH:   np = do_switch(); break;
	case IF:       np = do_if();     break;
	case FOR:      np = do_for();    break;
	case DO:       np = do_do();     break;
	case WHILE:    np = do_while();  break;
	case CONTINUE:
	case BREAK:
	case RETURN:
	case GOTO:     np = do_goto();   break;
	case CASE:
		/* TODO */
	case DEFAULT:
		/* TODO */
		break;
	case IDEN:
		/* TODO: check if it can be a label */
	default: np = expr(); break;

	}
	expect(';');
	return np;
}

struct node *
compound(void)
{
	register struct node *np = nodecomp();

	expect('{');
	new_ctx();
	while (decl())
		/* nothing */;
	while (!accept('}'))
		addstmt(np, stmt());
	del_ctx();

	prtree(np);
	putchar('\n');
	return np;
}
