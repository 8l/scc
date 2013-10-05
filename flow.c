
#include <assert.h>
#include <stdio.h>

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

static struct symbol*
newlabel(struct symbol *sym, char *s)
{
	switch (sym->ns) {
	case NS_LABEL:
		error("label '%s' already defined", yytext);
	case NS_ANY:
		sym->ns = NS_LABEL;
		break;
	default:
		lookup(s, NS_LABEL);
	}

	insert(sym, CTX_FUNC);
	return sym;
}

static struct node *
_goto(void)
{
	register struct node *np;
	register struct symbol *sym;


	expect(GOTO);
	expect(IDEN);
	sym = yyval.sym;
	if (sym->ns != NS_LABEL)
		sym = newlabel(sym, yytext);
	np = node(OGOTO, nodesym(sym), NULL);
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
	np = node(OWHILE, cond, stmt());
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
	np = node(ODO, body, cond);
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
	np = node(OFOR, exp1,
	          node(O2EXP, exp2,
	               node(O2EXP, exp3, stmt())));
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

	return node(OIF, cond,
	            node(O2EXP, body, (accept(ELSE)) ? stmt() : NULL));
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
	np = node(OSWITCH, cond, stmt());
	pop();
	return np;
}

static struct node *
label(void)
{
	register struct symbol *sym = yyval.sym;

	sym = newlabel(sym, yytext);
	next(), next();  	/* skip IDEN and ':' */
	return node(OLABEL, nodesym(sym), stmt());
}

static struct node *
_break(void)
{
	expect(BREAK);
	expect(';');
	if (blockp == blocks)
		error("break statement not within loop or switch");
	return node(OBREAK, NULL, NULL);
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

	return node(OCONT, NULL, NULL);
}

static struct node *
_return(void)
{
	register struct node *np;

	expect(RETURN);
	np = expr();
	expect(';');

	return node(ORETURN, np, NULL);
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
	np = node(OCASE, exp, NULL);
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
	return node(ODEFAULT, NULL, NULL);
}

static struct node *
compound(void)
{
	register struct node *np;
	struct compound c;

	nodecomp(&c);
	expect('{');
	new_ctx();
	while (np = decl())
		addstmt(&c, np);
	while (!accept('}'))
		addstmt(&c, stmt());
	del_ctx();

	return c.tree;
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
