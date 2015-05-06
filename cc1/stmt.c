
#include <stddef.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>

#include "../inc/cc.h"
#include "cc1.h"

struct scase {
	Symbol *label;
	Node *expr;
	struct scase *next;
};

struct caselist {
	short nr;
	Symbol *deflabel;
	struct scase *head;
};

Symbol *curfun;

extern Node *convert(Node *np, Type *tp1, char iscast);
extern Node *iszero(Node *np), *eval(Node *np);
static void stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

static Symbol *
label(char *s, char define)
{
	Symbol *sym;

	if ((sym = lookup(s, NS_LABEL)) != NULL) {
		if (define) {
			if (sym->isdefined)
				error("label '%s' already defined", s);
			sym->isdefined = 1;
		}
		return sym;
	}

	sym = install(s, NS_LABEL);
	sym->isdefined = define;
	return sym;
}

static void
stmtexp(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Node *np = NULL;;

	if (yytoken != ';') {
		np = expr();
		emit(OEXPR, np);
	}

	expect(';');
	freetree(np);
}

static Node *
condition(void)
{
	extern jmp_buf recover;
	extern Symbol *zero;
	Node *np;

	expect('(');
	setsafe(END_COND);
	if (!setjmp(recover))
		np = iszero(expr());
	else
		np = symbol(zero);
	expect(')');
	return np;
}

static void
While(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *cond, *end;
	Node *np;

	begin = install("", NS_LABEL);
	end = install("", NS_LABEL);
	cond = install("", NS_LABEL);

	expect(WHILE);
	np = condition();
	emit(OJUMP, cond);
	emitbloop();
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	emit(OLABEL, cond);
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emiteloop();
	emit(OLABEL, end);
	freetree(np);
}

static void
For(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *cond, *end;
	Node *econd, *einc, *einit;

	begin = install("", NS_LABEL);
	end = install("", NS_LABEL);
	cond = install("", NS_LABEL);

	expect(FOR);
	expect('(');
	einit = (yytoken != ';') ? expr() : NULL;
	expect(';');
	econd = (yytoken != ';') ? expr() : NULL;
	expect(';');
	einc = (yytoken != ')') ? expr() : NULL;
	expect(')');

	emit(OEXPR, einit);
	emit(OJUMP, cond);
	emitbloop();
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	emit(OEXPR, einc);
	emit(OLABEL, cond);
	emit(OBRANCH, begin);
	emit(OEXPR, econd);
	emiteloop();
	emit(OLABEL, end);
	freetree(einit);
	freetree(econd);
	freetree(einc);
}

static void
Dowhile(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *end;
	Node *np;

	begin = install("", NS_LABEL);
	end = install("", NS_LABEL);
	expect(DO);
	emitbloop();
	emit(OLABEL, begin);
	stmt(end, begin, lswitch);
	expect(WHILE);
	np = condition();
	emit(OBRANCH, begin);
	emit(OEXPR, np);
	emiteloop();
	emit(OLABEL, end);
	freetree(np);
}

static void
Return(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Node *np;
	Type *tp = curfun->type->type;

	expect(RETURN);
	np = (yytoken != ';') ? eval(expr()) : NULL;
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
	emitret(tp);
	emit(OEXPR, np);
	freetree(np);
}

static void
Break(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	expect(BREAK);
	if (!lbreak)
		error("break statement not within loop or switch");
	emit(OJUMP, lbreak);
	expect(';');
}

static void stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

static void
Label(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	switch (yytoken) {
	case IDEN: case TYPEIDEN:
		emit(OLABEL, label(yytext, 1));
		next();
		expect(':');
		stmt(lbreak, lcont, lswitch);
		break;
	default:
		unexpected();
	}
}

static void
Continue(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	expect(CONTINUE);
	if (!lcont)
		error("continue statement not within loop");
	emit(OJUMP, lcont);
	expect(';');
}

static void
Goto(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	expect(GOTO);

	if (yytoken != IDEN)
		unexpected();
	emit(OJUMP, label(yytext, 0));
	next();
	expect(';');
}

static void
Switch(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Caselist lcase = {.nr = 0, .head = NULL, .deflabel = NULL};
	struct scase *p, *next;
	Node *cond;
	Symbol *lcond;
	void free(void *ptr);

	expect(SWITCH);
	expect ('(');
	cond = expr();
	if ((cond = convert(cond, inttype, 0)) == NULL)
		error("incorrect type in switch statement");
	expect (')');

	lbreak = install("", NS_LABEL);
	lcond = install("", NS_LABEL);
	emit(OJUMP, lcond);
	stmt(lbreak, lcont, &lcase);
	emit(OLABEL, lcond);
	emitswitch(lcase.nr);
	emit(OEXPR, cond);
	for (p = lcase.head; p; p = next) {
		emit(OCASE, p->label);
		emit(OEXPR, p->expr);
		next = p->next;
		freetree(p->expr);
		free(p);
	}
	if (lcase.deflabel)
		emit(ODEFAULT, lcase.deflabel);
	emit(OLABEL, lbreak);
	freetree(cond);
}

static void
Case(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Node *np;
	struct scase *pcase;

	expect(CASE);
	if (!lswitch)
		error("case label not within a switch statement");
	np = expr();
	if ((np = convert(np, inttype, 0)) == NULL)
		error("incorrect type in case statement");
	expect(':');
	pcase = xmalloc(sizeof(*pcase));
	pcase->expr = np;
	pcase->next = lswitch->head;
	emit(OLABEL, pcase->label = install("", NS_LABEL));
	lswitch->head = pcase;
	++lswitch->nr;
}

static void
Default(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *ldefault = install("", NS_LABEL);

	expect(DEFAULT);
	expect(':');
	emit(OLABEL, ldefault);
	lswitch->deflabel = ldefault;
}

static void
If(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *end, *lelse;
	Node *np;

	lelse = install("", NS_LABEL);
	expect(IF);
	np = condition();
	NEGATE(np, 1);
	emit(OBRANCH, lelse);
	emit(OEXPR, np);
	stmt(lbreak, lcont, lswitch);
	if (accept(ELSE)) {
		end = install("", NS_LABEL);
		emit(OJUMP, end);
		emit(OLABEL, lelse);
		stmt(lbreak, lcont, lswitch);
		emit(OLABEL, end);
	} else {
		emit(OLABEL, lelse);
	}
	freetree(np);
}

void
compound(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	extern jmp_buf recover;
	pushctx();
	expect('{');

	for (;;) {
		setsafe(END_COMP);
		setjmp(recover);
		switch (yytoken) {
		case '}':
			goto end_compound;
		case TYPEIDEN:
			if (ahead() == ':')
				goto statement;
			/* pass through */
		case TYPE: case SCLASS: case TQUALIFIER:
			decl();
			break;
		default:
		statement:
			stmt(lbreak, lcont, lswitch);
		}
	}

end_compound:
	popctx();
	expect('}');
	return;
}

static void
stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	void (*fun)(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);
	Node *np;

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
	case TYPEIDEN: case IDEN:
		fun = (ahead() == ':') ? Label : stmtexp;
		break;
	case '@':
		next();
		np = expr();
		emitprint(np);
		freetree(np);
		return;
	}
	(*fun)(lbreak, lcont, lswitch);
}
