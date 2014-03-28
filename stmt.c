
#include <stdint.h>

#include "symbol.h"
#include "tokens.h"


void
compound(void)
{
	extern void decl(void);

	expect('{');
	while (!accept('}')) {
		switch (yytoken) {
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
			expr();
			/* TODO: Evaluate the expression here */
		}
		expect(';');
	}
}