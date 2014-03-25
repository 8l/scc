
#include <stdint.h>

#include "symbol.h"
#include "tokens.h"


void
compound(void)
{
	extern struct node *expr(void);
	extern void decl(void);

	expect('{');
	while (!accept('}')) {
		switch (yytoken) {
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
			eval(expr());
		}
		expect(';');
	}
}