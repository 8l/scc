
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
	register struct node *e1;

	expect(SWITCH);
	expect('(');
	e1 = expr();
	expect(')');
	return node2(OSWITCH, e1, stmt());
}

static struct node *
stmt(void)
{
	register struct node *np;

	switch (yytoken) {
	case '{':      return compound();
	case SWITCH:   return do_switch();
	case IF:       return do_if();
	case FOR:      return do_for();
	case DO:       return do_do();
	case WHILE:    return do_while();
	case CONTINUE:
	case BREAK:
	case RETURN:
	case GOTO:     return do_goto();
	case CASE:     /* TODO */
	case DEFAULT:  /* TODO */
	case IDEN:     /* TODO: check if it can be a label */;
	}
	np = expr();
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

	return np;
}
