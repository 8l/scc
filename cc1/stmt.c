
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"
#include "arch.h"
#include "cc1.h"

Symbol *curfun;

static void stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

static void
label(void)
{
	Symbol *sym;

	switch (yytoken) {
	case IDEN:
	case TYPEIDEN:
		sym = lookup(NS_LABEL, yytext);
		if (sym->flags & ISDEFINED)
			error("label '%s' already defined", yytext);
		if ((sym->flags & ISDECLARED) == 0)
			sym = install(NS_LABEL, sym);
		sym->flags |= ISDEFINED;
		emit(OLABEL, sym);
		next();
		expect(':');
		break;
	default:
		unexpected();
	}
}

static void
stmtexp(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	if (accept(';'))
		return;
	if (yytoken == IDEN && ahead() == ':') {
		label();
		stmt(lbreak, lcont, lswitch);
		return;
	}
	emit(OEXPR, expr());
	expect(';');
}

static Node *
condition(void)
{
	Node *np;

	expect('(');
	np = condexpr();
	expect(')');

	return np;
}

static void
While(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *cond, *end;
	Node *np;

	begin = newlabel();
	end = newlabel();
	cond = newlabel();

	expect(WHILE);
	np = condition();
	emit(OJUMP, cond);
	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	emit(OLABEL, cond);
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emit(OELOOP, NULL);
	emit(OLABEL, end);
}

static void
For(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *cond, *end;
	Node *econd, *einc, *einit;

	begin = newlabel();
	end = newlabel();
	cond = newlabel();

	expect(FOR);
	expect('(');
	einit = (yytoken != ';') ? expr() : NULL;
	expect(';');
	econd = (yytoken != ';') ? condexpr() : NULL;
	expect(';');
	einc = (yytoken != ')') ? expr() : NULL;
	expect(')');

	emit(OEXPR, einit);
	emit(OJUMP, cond);
	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	emit(OEXPR, einc);
	emit(OLABEL, cond);
	emit((econd) ? OBRANCH : OJUMP, begin);
	emit(OEXPR, econd);
	emit(OELOOP, NULL);
	emit(OLABEL, end);
}

static void
Dowhile(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *end;
	Node *np;

	begin = newlabel();
	end = newlabel();
	expect(DO);
	emit(OBLOOP, NULL);
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	expect(WHILE);
	np = condition();
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emit(OELOOP, NULL);
	emit(OLABEL, end);
}

static void
Return(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
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
			error("incorrect type in return");
	}
	emit(ORET, NULL);
	emit(OEXPR, np);
}

static void
Break(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
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
Continue(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
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
Goto(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *sym;

	namespace = NS_LABEL;
	next();
	namespace = NS_IDEN;

	if (yytoken != IDEN)
		unexpected();
	sym = yylval.sym;
	if ((sym->flags & ISDECLARED) == 0)
		sym = install(NS_LABEL, sym);
	sym->flags |= ISUSED;
	emit(OJUMP, sym);
	next();
	expect(';');
}

static void
Switch(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Caselist lcase = {.nr = 0, .head = NULL, .deflabel = NULL};
	Node *cond;

	expect(SWITCH);
	expect ('(');

	if ((cond = convert(expr(), inttype, 0)) == NULL) {
		errorp("incorrect type in switch statement");
		cond = constnode(zero);
	}
	expect (')');

	lcase.expr = cond;
	lcase.lbreak = newlabel();
	lcase.ltable = newlabel();

	emit(OSWITCH, &lcase);
	stmt(lbreak, lcont, &lcase);
	emit(OJUMP, lcase.lbreak);
	emit(OLABEL, lcase.ltable);
	emit(OSWITCHT, &lcase);
	emit(OLABEL, lcase.lbreak);
}

static void
Case(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Node *np;
	struct scase *pcase;

	expect(CASE);
	if (!lswitch)
		errorp("case label not within a switch statement");
	if ((np = iconstexpr()) == NULL)
		errorp("case label does not reduce to an integer constant");
	expect(':');
	if (lswitch && lswitch->nr >= 0) {
		if (++lswitch->nr == NR_SWITCH) {
			errorp("too case labels for a switch statement");
			lswitch->nr = -1;
		} else {
			pcase = xmalloc(sizeof(*pcase));
			pcase->expr = np;
			pcase->next = lswitch->head;
			emit(OLABEL, pcase->label = newlabel());
			lswitch->head = pcase;
		}
	}
	stmt(lbreak, lcont, lswitch);
}

static void
Default(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *ldefault = newlabel();

	expect(DEFAULT);
	expect(':');
	emit(OLABEL, ldefault);
	lswitch->deflabel = ldefault;
	++lswitch->nr;
	stmt(lbreak, lcont, lswitch);
}

static void
If(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *end, *lelse;
	Node *np;

	lelse = newlabel();
	expect(IF);
	np = condition();
	emit(OBRANCH, lelse);
	emit(OEXPR, negate(np));
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
blockit(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
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
compound(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	static int nested;

	pushctx();
	expect('{');

	if (nested == NR_BLOCK)
		error("too nesting levels of compound statements");

	++nested;
	for (;;) {
		if (yytoken == '}')
			break;
		blockit(lbreak, lcont, lswitch);
	}
	--nested;

	popctx();
	/*
	 * curctx == GLOBALCTX+1 means we are at the end of a function
	 * so we have to pop the context related to the parameters
	 */
	if (curctx == GLOBALCTX+1)
		popctx();
	expect('}');
}

static void
stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	void (*fun)(Symbol *, Symbol *, Caselist *);

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
	case SWITCH:   fun = Switch;   break;
	case CASE:     fun = Case;     break;
	case DEFAULT:  fun = Default;  break;
	default:       fun = stmtexp;  break;
	}
	(*fun)(lbreak, lcont, lswitch);
}
