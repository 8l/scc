
#include <inttypes.h>
#include <stddef.h>

#include "../inc/cc.h"
#include "cc2.h"

void
peephole(void)
{
	Addr to, from;
	TINT i;
	uint8_t op;

	for (pc = prog; pc; pc = pc->next) {
		to = pc->to;
		from = pc->from;

		switch (pc->op) {
		case SUB:
		case ADD:
			if (from.kind == CONST) {
				if ((i = from.u.i) == 0 || i < 4) {
					delcode();
					op = (pc->op == ADD) ? INC : DEC;

					while (i--)
						inscode(op, &to, NULL);
				}
			/* TODO: More optimizations (ex: -1) */
			}
			break;
		case JP:
			if (to.u.sym->u.pc == pc->next)
				delcode();
			break;
		}
	}
}
