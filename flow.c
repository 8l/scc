
#include <assert.h>
#include <stdio.h>

#include "symbol.h"
#include "tokens.h"
#include "syntax.h"
#include "sizes.h"

static struct node *stmt(void);


static unsigned char blocks[NR_BLOCK];
static unsigned char *blockp = blocks;

static void
push(register unsigned char b)
{
	if (blockp == &blocks[NR_BLOCK])
		error("Too much nesting levels");
	*blockp++ = b;
}

static void
pop(void)
{
	assert(blockp >= blocks);
	--blockp;
}

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
	register struct node *cond, *np;

	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	push(OWHILE);
	np = node2(OWHILE, cond, stmt());
	pop();
	return np;
}

static struct node *
_do(void)
{
	register struct node *cond, *body, *np;

	expect(DO);
	body = stmt();
	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	expect(';');

	push(ODO);
	np = node2(ODO, body, cond);
	pop();
	return np;
}

static struct node *
_for(void)
{
	register struct node *exp1, *exp2, *exp3;
	struct node *np;

	expect(FOR);
	expect('(');
	exp1 = (yytoken != ';') ? expr() : NULL;
	expect(';');
	exp2 = (yytoken != ';') ? expr() : NULL;
	expect(';');
	exp3 = (yytoken != ')') ? expr() : NULL;
	expect(')');

	push(OFOR);
	np = node2(OFOR, node3(OFEXP, exp1, exp2, exp3), stmt());
	pop();
	return np;
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
	register struct node *cond, *np;

	expect(SWITCH);
	expect('(');
	cond = expr();
	expect(')');

	push(OSWITCH);
	np = node2(OSWITCH, cond, stmt());
	pop();
	return np;
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
_break(void)
{
	expect(BREAK);
	expect(';');
	if (blockp == blocks)
		error("break statement not within loop or switch");
	return node1(OBREAK, NULL);
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
	case BREAK:    return _break();
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
