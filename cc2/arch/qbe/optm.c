
#include <stddef.h>

#include "arch.h"
#include "../../cc2.h"

Node *
optm(Node *np)
{
	int op = np->op;
	Node *p, *dst, *next = np->next;
	Symbol *sym, *osym;

	if (!next) {
		/*
		 * In QBE we need at the end of a basic block
		 * a jump, so we have to ensure that the last
		 * statement of the function is a ret, a jmp
		 * or a branch. In the same way, QBE does
		 * not accept labels at the end of a function
		 * (ONOP is used for labels) so we have to add
		 * a ret there, and in the case of branches
		 * we need a label for the next statement
		 */
		if (op == ONOP || op == OBRANCH || (op != ORET && op != OJMP))
			addstmt(newnode(ORET), KEEPCUR);
		next = np->next;
	}

	switch (op) {
	case ONOP:
		if (next->op == ONOP) {
			sym = np->u.sym;
			osym = next->u.sym;
			osym->id = sym->id;
			osym->numid = sym->id;
			osym->u.stmt = sym->u.stmt;
			return NULL;
		}
		break;
	case OBRANCH:
		if (!next->label) {
			sym = getsym(TMPSYM);
			sym->kind = SLABEL;
			next->label = sym;
		}
	case OJMP:
		for (;;) {
			dst = np->u.sym->u.stmt;
			if (dst->op != OJMP)
				break;
			np->u.sym = dst->u.sym;
		}
		for (p = np->next; p; p = p->next) {
			if (p == dst)
				return NULL;
			if (p->op == ONOP ||
			    p->op == OBLOOP ||
			    p->op == OELOOP) {
				continue;
			}
			break;
		}
		break;
	}
	return np;
}
