/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc1/stmt.c";
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>

#include <cstd.h>
#include "../inc/cc.h"
#include "cc1.h"

#define NEGATE   1
#define NONEGATE 0

Symbol *curfun;

static void stmt(Symbol *lbreak, Symbol *lcont, Switch *lswitch);

static void
label(void)
{
	Symbol *sym;

	switch (yytoken) {
	case IDEN:
	case TYPEIDEN:
		sym = lookup(NS_LABEL, yytext, ALLOC);
		if (sym->flags & SDEFINED)
			error("label '%s' already defined", yytext);
		if ((sym->flags & SDECLARED) == 0)
			sym = install(NS_LABEL, sym);
		sym->flags |= SDEFINED;
		emit(OLABEL, sym);
		next();
		expect(':');
		break;
	default:
		unexpected();
	}
}

static void
stmtexp(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Node *np;

	if (accept(';'))
		return;
	if (yytoken == IDEN && ahead() == ':') {
		label();
		stmt(lbreak, lcont, lswitch);
		return;
	}
	np = expr();
	if ((np->flags & NEFFECT) == 0)
		warn("expression without side effects");
	emit(OEXPR, np);
	expect(';');
}

static Node *
condition(int neg)
{
	Node *np;

	expect('(');
	np = condexpr(neg);
	expect(')');

	return np;
}

static void
While(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Symbol *begin;
	Node *np;

	begin = newlabel();
	lcont = newlabel();
	lbreak = newlabel();

	expect(WHILE);
	np = condition(NONEGATE);

	emit(OJUMP, lcont);

	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(lbreak, lcont, lswitch);
	emit(OLABEL, lcont);
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emit(OELOOP, NULL);

	emit(OLABEL, lbreak);
}

static void
For(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Symbol *begin, *cond;
	Node *econd, *einc;

	begin = newlabel();
	lcont = newlabel();
	cond = newlabel();
	lbreak = newlabel();

	pushctx();

	expect(FOR);
	expect('(');
	switch (yytoken) {
	case TYPE:
	case TYPEIDEN:
	case TQUALIFIER:
	case SCLASS:
		decl();
		break;
	default:
		emit(OEXPR, expr());
	case ';':
		expect(';');
		break;
	}
	econd = (yytoken != ';') ? condexpr(NONEGATE) : NULL;
	expect(';');
	einc = (yytoken != ')') ? expr() : NULL;
	expect(')');

	emit(OJUMP, cond);

	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(lbreak, lcont, lswitch);
	emit(OLABEL, lcont);
	emit(OEXPR, einc);
	emit(OLABEL, cond);
	emit((econd) ? OBRANCH : OJUMP, begin);
	emit(OEXPR, econd);
	emit(OELOOP, NULL);

	emit(OLABEL, lbreak);

	popctx();
}

static void
Dowhile(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Symbol *begin;
	Node *np;

	begin = newlabel();
	lcont = newlabel();
	lbreak = newlabel();

	expect(DO);

	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(lbreak, lcont, lswitch);
	expect(WHILE);
	np = condition(NONEGATE);
	emit(OLABEL, lcont);
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emit(OELOOP, NULL);

	emit(OLABEL, lbreak);
}

static void
Return(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Node *np;
	Type *tp = curfun->type->type;

	expect(RETURN);
	np = (yytoken != ';') ? decay(expr()) : NULL;
	expect(';');
	if (!np) {
		if (tp != voidtype)
			warn("function returning non void returns no value");
		tp = voidtype;
	} else if (np->type != tp) {
		if (tp == voidtype)
			warn("function returning void returns a value");
		else if ((np = convert(np, tp, 0)) == NULL)
			errorp("incorrect type in return");
	}
	emit(ORET, NULL);
	emit(OEXPR, np);
}

static void
Break(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	expect(BREAK);
	if (!lbreak) {
		errorp("break statement not within loop or switch");
	} else {
		emit(OJUMP, lbreak);
		expect(';');
	}
}

static void
Continue(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	expect(CONTINUE);
	if (!lcont) {
		errorp("continue statement not within loop");
	} else {
		emit(OJUMP, lcont);
		expect(';');
	}
}

static void
Goto(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Symbol *sym;

	namespace = NS_LABEL;
	next();
	namespace = NS_IDEN;

	if (yytoken != IDEN)
		unexpected();
	sym = yylval.sym;
	if ((sym->flags & SDECLARED) == 0)
		sym = install(NS_LABEL, sym);
	sym->flags |= SUSED;
	emit(OJUMP, sym);
	next();
	expect(';');
}

static void
Swtch(Symbol *obr, Symbol *lcont, Switch *osw)
{
	Switch sw = {0};
	Node *cond;
	Symbol *lbreak;

	expect(SWITCH);

	expect ('(');
	if ((cond = convert(expr(), inttype, 0)) == NULL) {
		errorp("incorrect type in switch statement");
		cond = constnode(zero);
	}
	expect (')');

	lbreak = newlabel();
	emit(OBSWITCH, NULL);
	emit(OEXPR, cond);
	stmt(lbreak, lcont, &sw);
	emit(OESWITCH, lbreak);
	emit(OLABEL, lbreak);
}

static void
Case(Symbol *lbreak, Symbol *lcont, Switch *sw)
{
	Node *np;
	Symbol *label;

	expect(CASE);
	if ((np = constexpr()) == NULL)
		errorp("case label does not reduce to an integer constant");
	if (!sw) {
		errorp("case label not within a switch statement");
	} else if (sw->nr >= 0 && ++sw->nr == NR_SWITCH) {
		errorp("too many case labels for a switch statement");
		sw->nr = -1;
	}
	expect(':');

	label = newlabel();
	emit(OCASE, label);
	emit(OEXPR, np);
	emit(OLABEL, label);
	stmt(lbreak, lcont, sw);
}

static void
Default(Symbol *lbreak, Symbol *lcont, Switch *sw)
{
	Symbol *label = newlabel();

	if (sw->hasdef)
		errorp("multiple default labels in one switch");
	sw->hasdef = 1;
	expect(DEFAULT);
	expect(':');
	emit(ODEFAULT, label);
	emit(OLABEL, label);
	stmt(lbreak, lcont, sw);
}

static void
If(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	Symbol *end, *lelse;
	Node *np;

	lelse = newlabel();
	expect(IF);
	np = condition(NEGATE);
	emit(OBRANCH, lelse);
	emit(OEXPR, np);
	stmt(lbreak, lcont, lswitch);
	if (accept(ELSE)) {
		end = newlabel();
		emit(OJUMP, end);
		emit(OLABEL, lelse);
		stmt(lbreak, lcont, lswitch);
		emit(OLABEL, end);
	} else {
		emit(OLABEL, lelse);
	}
}

static void
blockit(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	switch (yytoken) {
	case TYPEIDEN:
		if (ahead() == ':')
			goto parse_stmt;
		/* PASSTHROUGH */
	case TYPE:
	case TQUALIFIER:
	case SCLASS:
		decl();
		return;
	default:
	parse_stmt:
		stmt(lbreak, lcont, lswitch);
	}
}

void
compound(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	static int nested;

	pushctx();
	expect('{');

	if (nested == NR_BLOCK)
		error("too many nesting levels of compound statements");

	++nested;
	for (;;) {
		if (yytoken == '}')
			break;
		blockit(lbreak, lcont, lswitch);
	}
	--nested;

	popctx();
	expect('}');
}

static void
stmt(Symbol *lbreak, Symbol *lcont, Switch *lswitch)
{
	void (*fun)(Symbol *, Symbol *, Switch *);

	switch (yytoken) {
	case '{':      fun = compound; break;
	case RETURN:   fun = Return;   break;
	case WHILE:    fun = While;    break;
	case FOR:      fun = For;      break;
	case DO:       fun = Dowhile;  break;
	case IF:       fun = If;       break;
	case BREAK:    fun = Break;    break;
	case CONTINUE: fun = Continue; break;
	case GOTO:     fun = Goto;     break;
	case SWITCH:   fun = Swtch;    break;
	case CASE:     fun = Case;     break;
	case DEFAULT:  fun = Default;  break;
	default:       fun = stmtexp;  break;
	}
	(*fun)(lbreak, lcont, lswitch);
}
