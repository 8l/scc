
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "syntax.h"
#include "symbol.h"

struct node {
	unsigned char op;
};

struct node_op2 {
	struct node base;
	struct node *left;
	struct node *right;
};

struct nodesym {
	struct node base;
	struct symbol *sym;
};


struct node *
nodesym(struct symbol *sym)
{
	register struct nodesym *np = xmalloc(sizeof(*np));

	np->base.op = OSYM;
	np->sym = sym;
	return (struct node *) np;
}

struct node *
node(unsigned char op, struct node *l, struct node *r)
{
	register struct node_op2 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->right = r;

	return (struct node *) np;
}

bool
walk(register struct node *np, bool (*fun)(struct node *))
{
	struct node_op2 *p;

	if (!np || np->op == OSYM)
		return 1;

	p = (struct node_op2 *) np;
	return (*fun)(np) && walk(p->left, fun) && walk(p->right, fun);
}
