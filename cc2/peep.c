
#include <inttypes.h>
#include <stddef.h>

#include "../inc/cc.h"
#include "cc2.h"

void
peephole(void)
{
	Addr to, from;
	TINT i;

	for (pc = prog; pc; pc = pc->next) {
		to = pc->to;
		from = pc->from;

		switch (pc->op) {
		case ADD:
			if (from.kind == CONST) {
				if ((i = from.u.i) == 0 || i < 4) {
					delcode();

					while (i--)
						inscode(INC, &to, NULL);
				}
			/* TODO: More optimizations (ex: -1) */
			}
			break;
		}
	}
}
