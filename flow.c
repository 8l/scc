
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
	exp1 = expr();
	expect(';');
	exp2 = expr();
	expect(';');
	exp3 = expr();
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
_continue(void)
{
	register unsigned char *bp;

	expect(CONTINUE);
	expect(';');

	for (bp = blocks; bp < blockp && *bp == OSWITCH; ++bp)
		; /* nothing */
	if (bp == blockp)
		error("continue statement not within loop");

	return node1(OCONT, NULL);
}

static struct node *
_return(void)
{
	register struct node *np;
	extern struct ctype *curfun;

	expect(RETURN);
	/* TODO: Check the type of the function, can be void */
	np = expr();
	expect(';');
	return node1(ORETURN, np);
}

static struct node *
_case(void)
{
	register unsigned char *bp;
	register struct node *np, *exp;

	expect(CASE);
	exp = expr();
	/* TODO: check if exp is constant */
	/* TODO: Check if the type is correct */
	for (bp = blocks; bp < blockp && *bp != OSWITCH; ++bp)
		; /* nothing */
	if (bp == blockp)
		error("case statement not within switch");
	np = node1(OCASE, exp);
	expect(':');
	return np;
}

static struct node *
_default(void)
{
	register unsigned char *bp;

	expect(DEFAULT);
	for (bp = blocks; bp < blockp && *bp != OSWITCH; ++bp)
		; /* nothing */
	if (bp == blockp)
		error("default statement not within switch");
	expect(':');
	return node1(ODEFAULT, NULL);
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
	case CONTINUE: return _continue();
	case BREAK:    return _break();
	case RETURN:   return _return();
	case GOTO:     return _goto();
	case CASE:     return _case();
	case DEFAULT:  return _default();
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
