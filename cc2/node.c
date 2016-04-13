
#include <stdlib.h>

#include "../inc/cc.h"

#include "arch.h"
#include "cc2.h"

#define NSYMBOLS   32

Symbol *curfun;

struct arena {
	Node *mem;
	struct arena *next;
};

static struct arena *arena;
static Node *freep;
static int inhome;

Node *
newnode(void)
{
	struct arena *ap;
	Node *np;

	if (!freep) {
		ap = xmalloc(sizeof(*ap));
		ap->mem = xcalloc(NSYMBOLS, sizeof(Node));
		ap->next = arena;
		arena = ap;
		for (np = ap->mem; np < &ap->mem[NSYMBOLS-1]; ++np)
			np->left = np+1;
		np->left = NULL;
		freep = np;
	}

	np = freep;
	freep = np->left;

	np->right = NULL;
	np->left = NULL;
	np->stmt = NULL;
	np->label = NULL;
	return np;
}

void
delnode(Node *np)
{
	np->left = freep;
	freep = np;
}

void
deltree(Node *np)
{
	if (!np)
		return;
	deltree(np->left);
	deltree(np->right);
	delnode(np);
}

void
cleannodes(void)
{
	struct arena *ap, *next;

	for (ap = arena; ap; ap = next) {
		next = ap->next;
		free(ap->mem);
		free(ap);
	}
}
