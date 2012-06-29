
#include <stdarg.h>
#include <stddef.h>

#include "cc.h"
#include "syntax.h"

struct node {
	nodeop *op;
	struct node *child[];
};

#define issym(np) (!(np)->op)
#define getsym(np) ((struct symbol *)(np)->child[0])


static struct node *
node_alloc(unsigned char n)
{
	return xmalloc(sizeof(struct node) + sizeof(struct node *) * n);
}

static struct node *
node1(struct node *np1)
{
	register struct node *np = node_alloc(1);

	np->child[0] = np1;
	return np;
}

static struct node *
node2(struct node *np1, struct node *np2)
{
	register struct node *np =  node_alloc(2);

	np->child[0] = np1;
	np->child[1] = np2;
	return np;
}

static struct node *
node3(struct node *np1, struct node *np2, struct node *np3)
{
	register struct node *np =  node_alloc(2);

	np->child[0] = np1;
	np->child[1] = np2;
	np->child[2] = np3;
	return np;
}

struct node *
leaf(struct symbol *sym)
{
	register struct node *new = node1(NULL);

	new->child[0] = (struct node *) sym;
	return new;
}

struct node *
op1(nodeop op, struct node *np)
{
	register struct node *new = node1(np);

	new->op = op;
	return new;
}

struct node *
op2(nodeop op, struct node *np1, struct node *np2)
{
	register struct node *new = node2(np1, np2);

	new->op = op;
	return new;
}

struct node *
op3(nodeop op, struct node *np1, struct node *np2, struct node *np3)
{
	register struct node *new = node3(np1, np2, np3);

	new->op = op;
	return new;
}
