
#include <stdio.h>

#include "symbol.h"
#include "tokens.h"
#include "syntax.h"

static struct node *stmt(void);

static struct node *
_goto(void)
{
	register struct node *np;

	expect(GOTO);
	expect(IDEN);
	if (yyval.sym->ns != NS_LABEL)
		yyval.sym = lookup(yytext, NS_LABEL, CTX_ANY);
	np = node1(OGOTO, nodesym(yyval.sym));
	expect(';');

	return np;
}

static struct node *
_while(void)
{
	register struct node *cond;

	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	return node2(OWHILE, cond, stmt());
}

static struct node *
_do(void)
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
_for(void)
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
_if(void)
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
_switch(void)
{
	register struct node *cond;

	expect(SWITCH);
	expect('(');
	cond = expr();
	expect(')');
	return node2(OSWITCH, cond, stmt());
}

static struct node *
label(void)
{
	register struct symbol *sym;

	if (!ahead(':')) {
		register struct node *np = expr(); /* it is an identifier */
		expect(';');                       /* not a label */
		return np;
	}

	sym = lookup(yytext, NS_LABEL, CTX_ANY);
	if (sym->ctx != CTX_ANY)
		error("label '%s' already defined", yytext);

	sym->ctx = curctx;
	next(), next();  /* skip IDEN and ':' */
	return node2(OLABEL, nodesym(sym), stmt());
}

static struct node *
stmt(void)
{
	register struct node *np;

	switch (yytoken) {
	case '{':      return compound();
	case SWITCH:   return _switch();
	case IF:       return _if();
	case FOR:      return _for();
	case DO:       return _do();
	case WHILE:    return _while();
	case CONTINUE:
	case BREAK:
	case RETURN:
	case GOTO:     return _goto();
	case CASE:     /* TODO */
	case DEFAULT:  /* TODO */
	case IDEN:     return label();
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
