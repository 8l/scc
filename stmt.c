
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

Symbol *curfun;

extern Node *convert(Node *np, Type *tp1, char iscast);
static void stmt(Symbol *lbreak, Symbol *lcont, Symbol *lswitch);

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
	for (;;) {
		switch (yytoken) {
		case '}':
			next();
			return;
		case '{':
			compound(lbreak, lcont, lswitch);
			break;
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
			stmt(lbreak, lcont, lswitch);
		}
	}
}

static void
stmt(Symbol *lbreak, Symbol *lcont, Symbol *lswitch)
{
	switch (yytoken) {
	case '{': compound(lbreak, lcont, lswitch); break;
	case RETURN: Return(); break;
	default: emitexp(stmtexp()); break;
	}
}