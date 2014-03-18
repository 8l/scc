
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "syntax.h"
#include "symbol.h"

struct node {
	unsigned char op;
	union value u;
	struct ctype *type;
	struct node *left;
	struct node *right;
};

struct node *
node(unsigned char op, struct node *l, struct node *r)
{
	register struct node *np = xmalloc(sizeof(*np));

	np->op = op;
	np->left = l;
	np->right = r;

	return np;
}

bool
walk(register struct node *np, bool (*fun)(struct node *))
{
	if (!np)
		return 1;

	return (*fun)(np) && walk(np->left, fun) && walk(np->right, fun);
}
