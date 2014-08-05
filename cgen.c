
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "cc2.h"

struct addrtype {
	char op;
	char left;
	char right;
	char addrtype;
} addrtbl[] = {
	{'A', 0, 0, 1},
	{'#', 0, 0, 2},
	{'+', 1, 2, 3},
	{0},
};

struct nodeattr {
	char addrtype;
	char sethi;
};

struct nodeattr
genaddr(Node *np)
{
	struct nodeattr left, right;
	struct addrtype *bp;

	left = (np->left) ? genaddr(np->left) : (struct nodeattr) {0, -1};
	right = (np->right) ? genaddr(np->right) : (struct nodeattr) {0, -1};

	for (bp = addrtbl; bp->op; ++bp) {
		if (bp->op == np->op &&
		    left.addrtype == bp->left &&
		    right.addrtype == bp->right) {
			break;
		}
	}

	if ((np->addrtype = bp->addrtype) == 0) {
		np->sethi = 0;
	} else if (right.sethi < 0) {
		np->sethi = (left.sethi > 1) ? left.sethi : 1;
	} else  {
		int8_t d = left.sethi - right.sethi;
		np->sethi = ((d < 0) ? right.sethi : left.sethi) + 1;
	}

	return (struct nodeattr) {np->addrtype, np->sethi};
}

