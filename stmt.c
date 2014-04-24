
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

static void
stmtexp(void)
{
	if (accept(';'))
		return;
	emitexp(expr());
	expect(';');
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
	stmt(end, begin, lswitch);
	emitlabel(cond);
	emitjump(begin, np);
	emiteloop();
	emitlabel(end);
}

static void
For(Symbol *lswitch)
{
	Symbol *begin= label(NULL), *cond = label(NULL), *end = label(NULL);
	Node *econd = NULL, *einc = NULL;

	expect(FOR);
	expect('(');
	stmtexp();

	if (yytoken != ';')
		econd = expr();
	expect(';');
	if (yytoken != ')')
		einc = expr();
	expect(')');

	emitjump(cond, NULL);
	emitbloop();
	emitlabel(begin);
	stmt(end, begin, lswitch);
	if (einc)
		emitexp(einc);
	emitlabel(cond);
	emitjump(begin, econd);
	emiteloop();
	emitlabel(end);
}

static void
Dowhile(Symbol *lswitch)
{
	Symbol *begin= label(NULL), *end = label(NULL);


	expect(DO);
	emitbloop();
	emitlabel(begin);
	stmt(end, begin, lswitch);
	expect(WHILE);
	emitjump(begin, condition());
	emiteloop();
	emitlabel(end);
}

static void
Return(void)
{
	Node *np;
	Type *tp = curfun->type->type;

	expect(RETURN);
	np = expr();
	expect(';');
	if (np->type != tp) {
		if (tp == voidtype)
			warn(1, "function returning void returns a value");
		else if ((np = convert(np, tp, 0)) == NULL)
			error("incorrect type in return");
	}
	emitret(tp);
	emitexp(np);
}

static void
Break(Symbol *lbreak)
{
	expect(BREAK);
	if (!lbreak)
		error("break statement not within loop or switch");
	emitjump(lbreak, NULL);
	expect(';');
}

static void
Label(void)
{
	emitlabel(label(yytext));

	expect(IDEN);
	expect(':');
}

static void
Continue(Symbol *lcont)
{
	expect(CONTINUE);
	if (!lcont)
		error("continue statement not within loop");
	emitjump(lcont, NULL);
	expect(';');
}

static void
Goto(void)
{
	expect(GOTO);

	if (yytoken != IDEN)
		error("unexpected '%s'", yytext);
	emitjump(label(yytext), NULL);
	next();
	expect(';');
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

repeat:
	switch (yytoken) {
	case '{':      compound(lbreak, lcont, lswitch); break;
	case RETURN:   Return(); break;
	case WHILE:    While(lswitch); break;
	case FOR:      For(lswitch); break;
	case DO:       Dowhile(lswitch); break;
	case BREAK:    Break(lbreak); break;
	case CONTINUE: Continue(lcont); break;
	case GOTO:     Goto(); break;
	case IDEN:     if (ahead() == ':') Label(); goto repeat;
	default:       stmtexp(); break;
	}
}

