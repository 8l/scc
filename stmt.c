
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

Symbol *curfun;

extern Node *convert(Node *np, Type *tp1, char iscast);
extern Node *iszero(Node *np), *eval(Node *np);
static void stmt(Symbol *lbreak, Symbol *lcont, Symbol *lswitch);

static Symbol *
label(char *s, char define)
{
	Symbol *sym;

	if (s) {
		if ((sym = lookup(s, NS_LABEL)) != NULL) {
			if (define && sym->s.isdefined)
				error("label '%s' already defined", s);
			else
				sym->s.isdefined = 1;
			return sym;
		}
	} else {
		s = "";
	}

	sym = install(s, NS_LABEL);
	sym->s.isdefined = define;
	return sym;
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
	Symbol *begin, *cond, *end;
	Node *np;

	begin = label(NULL, 1);
	end = label(NULL, 1);
	cond = label(NULL, 1);

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
	Symbol *begin, *cond, *end;
	Node *econd = NULL, *einc = NULL;

	begin = label(NULL, 1);
	end = label(NULL, 1);
	cond = label(NULL, 1);

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
	Symbol *begin= label(NULL, 1), *end = label(NULL, 1);


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
	np  =  (yytoken == ';') ? NULL : eval(expr());
	expect(';');
	if (!np) {
		if (tp != voidtype)
			warn(1, "function returning non void returns no value");
		tp = voidtype;
	} else if (np->type != tp) {
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
	emitlabel(label(yytext, 1));

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
	emitjump(label(yytext, 0), NULL);
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
	case IDEN:
		if (ahead() == ':') {
			Label();
			goto repeat;
		}
	default:       stmtexp(); break;
	}
}

