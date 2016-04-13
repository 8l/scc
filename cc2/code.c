
#include <stdlib.h>

#include "arch.h"
#include "cc2.h"

static Inst *pc, *prog;

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
        new->to.kind = new->from2.kind = new->from1.kind = NONE;
        pc = new;
}

void
addr(int op, Node *np, Addr *addr)
{
	switch (addr->kind = np->op) {
	case REG:
		addr->u.reg = np->u.reg;
		break;
	case CONST:
		/* TODO: different type of constants*/
		np->u.i = np->u.i;
		break;
	case LABEL:
	case MEM:
		addr->u.sym = np->u.sym;
		break;
	case AUTO:
	case INDEX:
		break;
	default:
		abort();
	}

}

void
code(int op, Node *to, Node *from1, Node *from2)
{
	nextpc();
	if (from1)
		addr(op, from1, &pc->from1);
	if (from2)
		addr(op, from2, &pc->from2);
	if (to)
		addr(op, to, &pc->to);
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
