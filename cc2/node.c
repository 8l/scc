/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/node.c";
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"

#include "cc2.h"

#define NNODES   32

Node *curstmt;
Symbol *curfun;

struct arena {
	Node *mem;
	struct arena *next;
};

static struct arena *arena;
static Node *freep;

Node *
newnode(int op)
{
	struct arena *ap;
	Node *np;

	if (!freep) {
		ap = xmalloc(sizeof(*ap));
		ap->mem = xcalloc(NNODES, sizeof(Node));
		ap->next = arena;
		arena = ap;
		for (np = ap->mem; np < &ap->mem[NNODES-1]; ++np)
			np->left = np+1;
		np->left = NULL;
		freep = np;
	}

	np = freep;
	freep = np->left;

	memset(np, 0, sizeof(*np));
	np->op = op;

	return np;
}

#ifndef NDEBUG
#include <stdio.h>

static void
prnode(Node *np)
{
	if (np->left)
		prnode(np->left);
	if (np->right)
		prnode(np->right);
	fprintf(stderr, "\t%c%lu", np->op, np->type.size);
}

void
prtree(Node *np)
{
	prnode(np);
	putc('\n', stderr);
}

void
prforest(char *msg)
{
	Node *np;

	if (!curfun)
		return;

	fprintf(stderr, "%s {\n", msg);
	for (np = curfun->u.stmt; np; np = np->next)
		prtree(np);
	fputs("}\n", stderr);
}
#endif

Node *
addstmt(Node *np, int flag)
{
	if (curstmt)
		np->next = curstmt->next;
	np->prev = curstmt;

	if (!curfun->u.stmt)
		curfun->u.stmt = np;
	else
		curstmt->next = np;

	if (flag == SETCUR)
		curstmt = np;

	return np;
}

Node *
delstmt(void)
{
	Node *next, *prev;

	next = curstmt->next;
	prev = curstmt->prev;
	if (next)
		next->prev = prev;
	if (prev)
		prev->next = next;
	else
		curfun->u.stmt = next;
	deltree(curstmt);

	return curstmt = next;
}

Node *
nextstmt(void)
{
	return curstmt = curstmt->next;
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
	arena = NULL;
	freep = NULL;
	curstmt = NULL;
}

void
apply(Node *(*fun)(Node *))
{
	if (!curfun)
		return;
	curstmt = curfun->u.stmt;
	while (curstmt)
		(*fun)(curstmt) ? nextstmt() : delstmt();
}
