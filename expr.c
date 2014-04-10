#include <stdint.h>
#include <stdio.h>

#include "cc.h"

#define SWAP(x1, x2, t) (t = x1, x1 = x2, x2 = t)

Node *expr(void);

static Node *
primary(void)
{
	Node *np;
	Symbol *sym;

	switch (yytoken) {
	case IDEN:
		if ((sym = yylval.sym) == NULL)
			error("'%s' undeclared", yytext);
		np = node(emitsym, sym->type, SYM(sym), 0);
		np->b.lvalue = 1;
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

void
intconv(Node **np1, Node **np2)
{
}

void
floatconv(Node **np1, Node **np2)
{
}

static Node *
arithmetic(char op, Node *np1, Node *np2)
{
	Node *naux;
	Type *tp1, *tp2, *tp3;
	uint8_t t1, t2, taux, lvalue = 0;

	tp1 = UNQUAL(np1->type), tp2 = UNQUAL(np2->type);
	t1 = tp1->op, t2 = tp2->op;

	switch (t1) {
	case INT:
		switch (t2) {
		case INT:
			if (tp1 != tp2)
				intconv(&np1, &np2);
			break;
		case FLOAT:
			SWAP(np1, np2, naux);
			goto int_float;
		case PTR: case ARY:
			SWAP(np1, np2, naux);
			SWAP(t1, t2, taux);
			goto pointer;
		default:
			goto incorrect;
		}
		break;
	case FLOAT:
		switch (t2) {
		case FLOAT:
			if (tp1 != tp2)
				floatconv(&np1, &np2);
			break;
		case INT:
int_float:
			np2 = castcode(np2, np1->type);
			break;
		default:
			goto incorrect;
		}
		break;
	case PTR: case ARY:
pointer:
		if (!tp1->defined)
			goto nocomplete;
		if (op != OADD && op != OSUB)
			goto incorrect;
		tp3 = tp1->type;
		if (t1 == ARY)
			tp1 = mktype(tp1->type, PTR, NULL, 0);
		if (t2 != INT)
			goto incorrect;
		np2 = bincode(OMUL, tp1,
		              castcode(np2, tp1),
		              sizeofcode(tp3));
		lvalue = 1;
		break;
	default:
		goto incorrect;
	}

	np1 = bincode(op, tp1, np1, np2);
	np1->b.lvalue = lvalue;
	return np1;

nocomplete:
	error("invalid use of indefined type");
incorrect:
	error("incorrect arithmetic operands");
}

static Node *
array(Node *np1, Node *np2)
{
	Type *tp;
	uint8_t t1, t2;
	char *err;

	t1 = BTYPE(np1->type);
	t2 = BTYPE(np2->type);
	if (!isaddr(t1) && !isaddr(t2))
		goto bad_vector;
	if (t1 != INT && t2 != INT)
		goto bad_subs;
	np1 = arithmetic(OADD, np1, np2);
	return unarycode(OARY, np1->type->type , np1);

bad_vector:
	err = "subscripted value is neither array nor pointer nor vector";
	goto error;
bad_subs:
	err = "array subscript is not an integer";
error:
	error(err);
}

static Node *
incdec(Node *np, char op)
{
	Type *tp;
	char *err;

	if (!np->b.lvalue)
		goto nolvalue;
	tp = UNQUAL(np->type);
	if (isconst(np->type->op))
		goto const_mod;

	switch (tp->op) {
	case PTR:
		if (!tp->type->defined)
			goto nocomplete;
	case INT: case FLOAT:
		np = unarycode(op, np->type, np);
		np->b.lvalue = 0;
		return np;
	default:
		goto bad_type;
	}

nolvalue:
	err = "lvalue required in operation";
	goto error;
nocomplete:
	err = "invalid use of indefined type";
	goto error;
bad_type:
	err = "incorrect type in arithmetic operation";
	goto error;
const_mod:
	err = "const value modified";
error:
	error(err);
}

static Node *
postfix(void)
{
	Node *np1, *np2;

	np1 = primary();
	for (;;) {
		switch (yytoken) {
		case '[':
			next();
			np2 = expr();
			np1 = array(np1, np2);
			expect(']');
			break;
		case DEC: case INC:
			np1 = incdec(np1,  (yytoken == INC) ? OPINC : OPDEC);
			next();
			break;
		default:
			return np1;
		}
	}
}

static Node *
unary(void)
{
	Node *np;

	switch (yytoken) {
	case INC: case DEC:
		np = incdec(unary(), (yytoken == INC) ? OINC : ODEC);
		next();
		break;
	default:
		return postfix();
	}
}

static struct node *
cast(void)
{
	Type *tp;
	Node *np1, *np2;
	extern Type *typename(void);

	if (yytoken == '(') {
		switch(ahead()) {
		case TQUALIFIER: case TYPE:
			next();
			tp = typename();
			expect(')');
			np1 = cast();
			np2 = castcode(np1, tp);
			np2->b.lvalue = np1->b.lvalue;
			return np1;
		default:
			break;
		}
	}

	return unary();
}

Node *
expr(void)
{
	Node *np;

	do
		np = cast();
	while (yytoken == ',');
	return np;
}
