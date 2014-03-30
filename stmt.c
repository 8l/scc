
#include <stddef.h>
#include <stdint.h>

#include "cc.h"

void
compound(void)
{
	extern void decl(void);
	extern Node *expr(void);

	expect('{');
	while (!accept('}')) {
		switch (yytoken) {
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
			emitexp(expr());
		}
		expect(';');
	}
}
