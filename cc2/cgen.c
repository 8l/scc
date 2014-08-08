
#include <stdint.h>
#include <stdlib.h>

#include "cc2.h"


#include <stdio.h>

static void
emit(char what, void *data)
{
	switch (what) {
	case FUN:
		printf("%s:\n", data);
		break;
	default:
		fputs("internal error: incorrect emit\n", stderr);
		abort();
	}
}

void
cgen(Symbol *sym, Node *list[])
{
	emit(FUN, sym->u.f.name);
}

/*
 * calculate addresability as follows
 *     AUTO => 11
 *     REGISTER => 13
 *     STATIC => 12
 *     CONST => 20
 */
static void
xaddable(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return;

	np->complex = 0;
	np->addable = 0;
	lp = np->left;
	rp = np->right;
	switch (np->op) {
	case AUTO:
		np->addable = 11;
		break;
	case REG:
		np->addable = 13;
		break;
	case MEM:
		np->addable = 12;
		break;
	case CONST:
		np->addable = 20;
		break;
	case OADD:
	case OSUB:
		xaddable(lp);
		xaddable(rp);
	}
}

void
genaddable(Node *list[])
{
	Node *np;

	while ((np = *list++) != NULL)
		xaddable(np);
}
