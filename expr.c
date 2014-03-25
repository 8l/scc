#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "code.h"
#include "tokens.h"
#include "symbol.h"

struct node *expr(void);

enum {
	OSYM = 1, OARY, OADDR, OADD
};

struct node {
	uint8_t op;
	struct ctype *type;

	struct {
		bool constant : 1;
	} f;
	union  {
		struct symbol *sym;
	} u;
	struct node *left, *right;
};

struct node *
newnode(uint8_t op, struct ctype *tp)
{
	struct node *np = xcalloc(1, sizeof(*np));

	np->op = op;
	np->type = tp;
	return np;	
}

static struct node *
primary(void)
{
	register struct node *np;

	switch (yytoken) {
	case IDEN:
		if (yylval.sym == NULL)
			error("'%s' undeclared", yytext);
		np = newnode(OSYM, yylval.sym->type);
		np->u.sym = yylval.sym;
		next();
		break;
	case CONSTANT:
		next();
		/* TODO: do something */
		break;
	case '(':
		next();
		np = expr();
		expect(')');
		break;
	default:
		error("unexpected '%s'", yytext);
	}
	return np;
}

static struct node *
ary2ptr(struct node *np)
{
	struct ctype *tp = np->type;

	struct node *aux;

	tp = mktype(UNQUAL(tp)->type, PTR, NULL, 0);
	aux= newnode(OADDR, tp);
	aux->left = np;
	return aux;
}

static struct node *
int2ptr(struct node *np)
{
	return np;
}

static struct node *
ary(struct node *np1)
{
	struct node *np2,  *naux;
	struct ctype *tp;
	uint8_t t1, t2, taux;

	np2 = expr();
	expect(']');
	t1 = BTYPE(np1->type);
	t2 = BTYPE(np2->type);

	if (!isaddr(t1)) {
		taux = t1,   t1 = t2,   t2 = taux;
		naux = np1, np1 = np2, np2 = naux;
	}
	if (!isaddr(t1))
		error("expected array or pointer");
	if (isary(t1))
		np1 = ary2ptr(np1);
	if (!isarith(t2))
		error("array subscript is not an integer");

	tp = np1->type;
	tp = UNQUAL(tp);
	naux = newnode(OADD, tp);
	naux->left = np1;
	naux->right = int2ptr(np2);
	return naux;
}

static struct node *
postfix(void)
{
	struct node *np;

	np = primary();
	for (;;) {
		switch (yytoken) {
		case '[': next(); np = ary(np); break;
		default: return np;
		}
	}			
}

struct node *
expr(void)
{
	register struct node *np;

	do
		np = postfix();
	while (yytoken == ',');

	return np;
}

static void
evalnode(struct node *np)
{
	if (!np)
		return;

	switch (np->op) {
	case OSYM:
		emitsym(np->u.sym);
		break;
	case OARY:
		evalnode(np->left);
		evalnode(np->right);
		fputs("\t'", stdout);
		break;
	case OADDR:
		evalnode(np->left);
		evalnode(np->right);
		fputs("\t@", stdout);
		break;
	case OADD:
		evalnode(np->left);
		evalnode(np->right);
		fputs("\t+", stdout);
		break;
	}
}

void
eval(struct node *np)
{
	evalnode(np);
	putchar('\n');
}