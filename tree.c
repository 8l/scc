
#include <stdarg.h>
#include <stddef.h>

#include "cc.h"
#include "syntax.h"

struct node {
	unsigned char op;
};

struct node_op1 {
	struct node base;
	struct node *infix;
};

struct node_op2 {
	struct node base;
	struct node *left;
	struct node *rigth;
};

struct node_op3 {
	struct node base;
	struct node *left;
	struct node *infix;
	struct node *rigth;
};

struct node_sym {
	struct node base;
	struct symbol *sym;
};


#define OSYM 1

struct node *
nodesym(struct symbol *sym)
{
	register struct node_sym *np = xmalloc(sizeof(*np));

	np->base.op = OSYM;
	np->sym = sym;
	return (struct node *) np;
}

struct node *
node3(unsigned char op, struct node *l, struct node *i, struct node *r)
{
	register struct node_op3 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->infix = i;
	np->rigth = r;

	return (struct node *) np;
}

struct node *
node2(unsigned char op, struct node *l, struct node *r)
{
	register struct node_op2 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->rigth = r;

	return (struct node *) np;
}

struct node *
node1(unsigned char op, struct node *i)
{
	register struct node_op1 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->infix = i;

	return (struct node *) np;
}
