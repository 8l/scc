
#include <stdint.h>
#include <stdlib.h>

#include <cc.h>
#include "cc2.h"

#include <stdio.h>

void
genstack(Symbol *fun)
{
	Symbol *p;
	short siz;

	for (siz = 0, p = fun->u.f.vars; p; p = p->next)
		siz += p->u.v.type->size;
	fun->u.f.stack = siz;
}

static void
emit(char what, void *data)
{
	Symbol *sym, *p;

	switch (what) {
	case FUN:
		sym = data;
		printf("%s:\n"
		       "\tPUSH\tIX\n"
		       "\tLD\tIX,SP\n"
		       "\tLD\tHL,%d\n"
		       "\tADD\tHL,SP\n"
		       "\tLD\tSP,HL\n", sym->u.f.name, -sym->u.f.stack);
		return;
	case EFUN:
		puts("\tLD\tSP,IX\n"
		     "\tPOP\tIX\n"
		     "\tRET");
		return;
	default:
		fputs("internal error: incorrect emit\n", stderr);
		abort();
	}
}

void
cgen(Symbol *sym, Node *list[])
{
	emit(FUN, sym);
	emit(EFUN, NULL);
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
