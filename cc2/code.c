
#include <stdlib.h>

#include "arch.h"
#include "cc2.h"

Inst *pc, *prog;

static void
nextpc(void)
{
        Inst *new;

        new = malloc(sizeof(*new)); /* TODO: create an arena */

        if (!pc) {
                new->next = NULL;
                prog = new;
        } else {
                new->next = pc->next;
                pc->next = new;
        }

        new->prev = pc;
        new->to.kind = new->from2.kind = new->from1.kind = SNONE;
        pc = new;
}

static void
addr(Node *np, Addr *addr)
{
	switch (np->op) {
	case OREG:
		addr->kind = SREG;
		addr->u.reg = np->u.reg;
		break;
	case OCONST:
		addr->kind = OCONST;
		addr->u.i = np->u.i;
		break;
	case OLABEL:
		addr->kind = SLABEL;
		goto symbol;
	case OAUTO:
		addr->kind = SAUTO;
		goto symbol;
	case OTMP:
		addr->kind = STMP;;
	symbol:
		addr->u.sym = np->u.sym;
		break;
	}
}

Node *
label2node(Symbol *sym)
{
	Node *np;

	np = newnode(OLABEL);
	np->u.sym = sym;

	return np;
}

void
setlabel(Symbol *sym)
{
	if (!sym)
		return;
	code(0, NULL, NULL, NULL);
	pc->label = sym;
}

void
code(int op, Node *to, Node *from1, Node *from2)
{
	nextpc();
	if (from1)
		addr(from1, &pc->from1);
	if (from2)
		addr(from2, &pc->from2);
	if (to)
		addr(to, &pc->to);
	pc->op = op;
}

void
delcode(void)
{
        Inst *prev = pc->prev, *next = pc->next;

        free(pc);
        if (!prev) {
                pc = next;
                prog = NULL;
        } else {
                pc = prev;
                prev->next = next;
                if (next)
                        next->prev = prev;
        }
}
