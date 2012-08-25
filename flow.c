
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
	register struct node *cond;

	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	return node2(OWHILE, cond, stmt());
}

static struct node *
do_do(void)
{
	register struct node *cond, *body;

	expect(DO);
	body = stmt();
	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	expect(';');

	return node2(ODO, body, cond);
}

static struct node *
do_for(void)
{
	register struct node *exp1, *exp2, *exp3;

	expect(FOR);
	expect('(');
	exp1 = (yytoken != ';') ? expr() : NULL;
	expect(';');
	exp2 = (yytoken != ';') ? expr() : NULL;
	expect(';');
	exp3 = (yytoken != ')') ? expr() : NULL;
	expect(')');
	
	return node2(OFOR, node3(OFEXP, exp1, exp2, exp3), stmt());
}

static struct node *
do_if(void)
{
	register struct node *cond, *body;

	expect(IF);
	expect('(');
	cond = expr();
	expect(')');
	body = stmt();

	return node3(OIF, cond, body, (accept(ELSE)) ? stmt() : NULL);
}

static struct node *
do_switch(void)
{
	register struct node *cond;

	expect(SWITCH);
	expect('(');
	cond = expr();
	expect(')');
	return node2(OSWITCH, cond, stmt());
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
