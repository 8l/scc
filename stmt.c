
#include <stdint.h>

#include "symbol.h"
#include "tokens.h"


void
compound(void)
{
	extern struct ctype *expr(void);
	extern void decl(void);

	expect('{');
	while (!accept('}')) {
		switch (yytoken) {
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
			expr();
		}
		expect(';');
	}
}