
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "cc2.h"

/*
 * calculate addresability as follows
 *     AUTO => 11
 *     REGISTER => 13
 *     STATIC => 12
 *     CONST => 20
 */
void
genaddable(Node *np)
{
	if (!np)
		return;

	np->complex = 0;
	np->addable = 0;
	switch (np->op) {
	case AUTO:
		np->addable = 11;
		break;
	case REGISTER:
		np->addable = 13;
		break;
	case STATIC:
		np->addable = 12;
		break;
	case CONST:
		np->addable = 20;
		break;
	}
}
