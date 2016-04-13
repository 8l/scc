
#include "arch.h"
#include "../../cc2.h"

void
generate(void)
{
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps (/sys/doc/compiler.ps)
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REG => 13           reg
 *     STATIC => 12        (value)
 *     CONST => 20         $value
 */
static Node *
address(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return np;

	np->complex = 0;
	np->address = 0;
	lp = np->left;
	rp = np->right;
	switch (np->op) {
	case AUTO:
		np->address = 11;
		break;
	case REG:
		np->address = 13;
		break;
	case MEM:
		np->address = 12;
		break;
	case CONST:
		np->address = 20;
		break;
	default:
		if (lp)
			address(lp);
		if (rp)
			address(rp);
		break;
	}

	if (np->address > 10)
		return np;
	if (lp)
		np->complex = lp->complex;
	if (rp) {
		int d = np->complex - rp->complex;

		if (d == 0)
			++np->complex;
		else if (d < 0)
			np->complex = rp->complex;
	}
	if (np->complex == 0)
		++np->complex;
	return np;
}

void
addressability(void)
{
	Node *np;

	if (!curfun)
		return;

	for (np = curfun->u.label; np; np = np->stmt)
		address(np);
}
