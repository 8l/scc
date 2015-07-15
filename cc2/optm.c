
#include <stddef.h>
#include <inttypes.h>

#include "../inc/cc.h"
#include "cc2.h"


#include <stdio.h>

static Node *
optcasts(Node *np, Type *tp)
{
	if (!np)
		return NULL;

repeat:
	switch (np->op) {
	case OCAST:
		/* TODO: be careful with the sign */
		if (np->type.flags&INTF && np->type.size >= tp->size) {
			np = np->left;
			goto repeat;
		}
		break;
	case OASSIG:
		tp = &np->type;
		break;
	default:
		if (np->type.size > tp->size)
			np->type = *tp;
		break;
	}

	np->left = optcasts(np->left, tp);
	np->right = optcasts(np->right, tp);
	return np;
}

static Node *
opt(Node *np)
{
	np = optcasts(np, &np->type);
	return np;
}

void
optimize(void)
{
	apply(opt);
}
