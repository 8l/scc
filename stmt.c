
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

Symbol *curfun;

extern Node *convert(Node *np, Type *tp1, char iscast);
extern Node * iszero(Node *np);
static void stmt(Symbol *lbreak, Symbol *lcont, Symbol *lswitch);

static Symbol *
label(char *s)
{
	if (!s)
		s = "";
	else if (lookup(s, NS_LABEL))
		error("label '%s' already defined", s);

	return install(s, NS_LABEL);
}

static Node *
stmtexp(void)
{
	Node *np = expr();
	expect(';');
	return np;
}

static Node *
condition(void)
{
	Node *np;

	expect('(');
	np = iszero(expr());
	expect(')');
	return np;
}

static void
While(Symbol *lswitch)
{
	Symbol *begin= label(NULL), *cond = label(NULL), *end = label(NULL);
	Node *np;

	expect(WHILE);
	np = condition();
	emitjump(cond, NULL);
	emitbloop();
	emitlabel(begin);
	stmt(begin, end, lswitch);
	emitlabel(cond);
	emitjump(begin, np);
	emiteloop();
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
	case WHILE: While(lswitch); break;
	default: emitexp(stmtexp()); break;
	}
}

