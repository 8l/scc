
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "cc2.h"

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
	}
	return;
}
