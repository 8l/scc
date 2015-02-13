
#include <stddef.h>
#include <stdint.h>
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
		emitexp(np);
	}

	expect(';');
	freetree(np);
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
While(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *begin, *cond, *end;
	Node *np;

	begin = install("", NS_LABEL);
	end = install("", NS_LABEL);
	cond = install("", NS_LABEL);

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

	emitexp(einit);
	emitjump(cond, NULL);
	emitbloop();
	emitlabel(begin);
	stmt(end, begin, lswitch);
	emitexp(einc);
	emitlabel(cond);
	emitjump(begin, econd);
	emiteloop();
	emitlabel(end);
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
	emitlabel(begin);
	stmt(end, begin, lswitch);
	expect(WHILE);
	np = condition();
	emitjump(begin, np);
	emiteloop();
	emitlabel(end);
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
	freetree(np);
}

static void
Break(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	expect(BREAK);
	if (!lbreak)
		error("break statement not within loop or switch");
	emitjump(lbreak, NULL);
	expect(';');
}

static void stmt(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

static void
Label(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	switch (yytoken) {
	case IDEN: case TYPEIDEN:
		emitlabel(label(yytext, 1));
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
	emitjump(lcont, NULL);
	expect(';');
}

static void
Goto(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	expect(GOTO);

	if (yytoken != IDEN)
		unexpected();
	emitjump(label(yytext, 0), NULL);
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
	emitjump(lcond, NULL);
	stmt(lbreak, lcont, &lcase);
	emitlabel(lcond);
	emitswitch(lcase.nr, cond);
	for (p = lcase.head; p; p = next) {
		emitcase(p->label, p->expr);
		next = p->next;
		freetree(p->expr);
		free(p);
	}
	if (lcase.deflabel)
		emitdefault(lcase.deflabel);
	emitlabel(lbreak);
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
	emitlabel(pcase->label = install("", NS_LABEL));
	lswitch->head = pcase;
	++lswitch->nr;
}

static void
Default(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	Symbol *ldefault = install("", NS_LABEL);

	expect(DEFAULT);
	expect(':');
	emitlabel(ldefault);
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
	emitjump(lelse, np);
	stmt(lbreak, lcont, lswitch);
	if (accept(ELSE)) {
		end = install("", NS_LABEL);
		emitjump(end, NULL);
		emitlabel(lelse);
		stmt(lbreak, lcont, lswitch);
		emitlabel(end);
	} else {
		emitlabel(lelse);
	}
	freetree(np);
}

void
compound(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	pushctx();
	expect('{');

	for (;;) {
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

