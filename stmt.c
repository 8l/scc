
#include <stddef.h>
#include <stdint.h>

#include "cc1.h"

Symbol *curfun;

extern Node *convert(Node *np, Type *tp1, char iscast);

static Node *
stmtexp(void)
{
	Node *np = expr();
	expect(';');
	return np;
}

static void
Return(void)
{
	Node *np;
	Type *tp = curfun->type->type;

	expect(RETURN);
	np = stmtexp();
	if (np->type != tp) {
		if (tp == voidtype)
			warn(1, "function returning void returns a value");
		else if ((np = convert(np, tp, 0)) == NULL)
			error("incorrect type in return");
	}
	emitret(tp);
	emitexp(np);
}

void
compound(Symbol *lbreak, Symbol *lcont, Symbol *lswitch)
{
	expect('{');
	while (!accept('}')) {
		switch (yytoken) {
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		case RETURN:
			Return();
			break;
		default:
			emitexp(stmtexp());
		}
	}
}
