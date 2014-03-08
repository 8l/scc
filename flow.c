
#include <assert.h>
#include <stdio.h>

#include "cc.h"
#include "symbol.h"
#include "tokens.h"
#include "syntax.h"
#include "sizes.h"

static struct node *stmt(void);


static unsigned char blocks[NR_BLOCK];
static unsigned char *blockp = blocks;
static struct symbol *curfun;

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
Goto(void)
{
	register struct node *np;
	register struct symbol *sym;


	expect(GOTO);
	expect(IDEN);
	sym = lookup(yytext, NS_LABEL);
	np = node(OGOTO, nodesym(sym), NULL);
	expect(';');

	return np;
}

static struct node *
While(void)
{
	register struct node *cond, *np;

	expect(WHILE);
	expect('(');
	cond = expr();
	expect(')');
	push(OWHILE);
	np = node(OWHILE, cond, stmt());
	pop();
	return np;
}

static struct node *
Do(void)
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
	np = node(ODO, body, cond);
	pop();
	return np;
}

static struct node *
For(void)
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
	np = node(OFOR, exp1,
	          node(O2EXP, exp2,
	               node(O2EXP, exp3, stmt())));
	pop();
	return np;
}

static struct node *
If(void)
{
	register struct node *cond, *body;

	expect(IF);
	expect('(');
	cond = expr();
	expect(')');
	body = stmt();

	return node(OIF, cond,
	            node(O2EXP, body, (accept(ELSE)) ? stmt() : NULL));
}

static struct node *
Switch(void)
{
	register struct node *cond, *np;

	expect(SWITCH);
	expect('(');
	cond = expr();
	expect(')');

	push(OSWITCH);
	np = node(OSWITCH, cond, stmt());
	pop();
	return np;
}

static struct node *
label(void)
{
	register struct symbol *sym = lookup(yytext, NS_LABEL);

	if (sym->label)
		error("label '%s' already defined", yytext);
	insert(sym, CTX_FUNC);
	sym->label = 1;
	next(), next();  	/* skip IDEN and ':' */
	return node(OLABEL, nodesym(sym), stmt());
}

static struct node *
Break(void)
{
	expect(BREAK);
	expect(';');
	if (blockp == blocks)
		error("break statement not within loop or switch");
	return node(OBREAK, NULL, NULL);
}

static struct node *
Continue(void)
{
	register unsigned char *bp;

	expect(CONTINUE);
	expect(';');

	for (bp = blocks; bp < blockp && *bp == OSWITCH; ++bp)
		; /* nothing */
	if (bp == blockp)
		error("continue statement not within loop");

	return node(OCONT, NULL, NULL);
}

static struct node *
Return(void)
{
	register struct node *np;

	expect(RETURN);
	np = expr();
	expect(';');

	return node(ORETURN, np, NULL);
}

static struct node *
Case(void)
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
	np = node(OCASE, exp, NULL);
	expect(':');
	return np;
}

static struct node *
Default(void)
{
	register unsigned char *bp;

	expect(DEFAULT);
	for (bp = blocks; bp < blockp && *bp != OSWITCH; ++bp)
		; /* nothing */
	if (bp == blockp)
		error("default statement not within switch");
	expect(':');
	return node(ODEFAULT, NULL, NULL);
}

static struct node *
compound(void)
{
	register struct node *np;
	unsigned char nodecl = 0;
	struct compound c;

	c.tree = NULL;
	expect('{');
	new_ctx();
	while (!accept('}')) {
		if (np = decl(0)) {
			if (nodecl) {
				warn(options.mixdcls,
				     "mixed declarations and code");
			}
		} else {
			np = stmt();
			nodecl = 1;
		}
		addstmt(&c, np);
	}
	del_ctx();

	return c.tree;
}

static struct node *
stmt(void)
{
	register struct node *np;

	switch (yytoken) {
	case '{':      return compound();
	case SWITCH:   return Switch();
	case IF:       return If();
	case FOR:      return For();
	case DO:       return Do();
	case WHILE:    return While();
	case CONTINUE: return Continue();
	case BREAK:    return Break();
	case RETURN:   return Return();
	case GOTO:     return Goto();
	case CASE:     return Case();
	case DEFAULT:  return Default();
	case IDEN:     if (ahead() == ':') return label();
	}
	np = expr();
	expect(';');
	return np;
}

struct node *
function(register struct symbol *sym)
{
	curfun = sym;
	return node(OFTN, compound(), NULL);
}

void
run(register struct node *np)
{
	prtree(np);
	putchar('\n');
	freesyms();
}
