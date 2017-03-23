/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/arch/qbe/optm.c";

#include <stddef.h>

#include "../../../inc/cc.h"
#include "../../cc2.h"

Node *
optm_dep(Node *np)
{
	int op = np->op;
	Node *p, *dst, *next = np->next;
	Symbol *sym, *osym;

	switch (op) {
	case OEFUN:
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
		op = (np->prev) ? np->prev->op : 0;
		if (!op || op == ONOP || op == OBRANCH || (op != ORET && op != OJMP))
			addstmt(newnode(ORET), KEEPCUR);
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
