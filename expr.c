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
		;
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
	uint8_t t1, t2, taux;

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
		if (op != OADD && op != OSUB)
			goto incorrect;
		/* TODO: check that the the base type is a complete type */
		tp3 = tp1->type;
		if (t1 == ARY)
			tp1 = mktype(tp1->type, PTR, NULL, 0);
		if (t2 != INT)
			goto incorrect;
		np2 = bincode(OMUL, tp1,
		              castcode(np2, tp1),
		              sizeofcode(tp3));
		break;
	default:
		goto incorrect;
	}

	return bincode(op, tp1, np1, np2);

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
postfix(void)
{
	Node *np1, *np2;
	char op;

	np1 = primary();
	for (;;) {
		switch (yytoken) {
		case '[':
			next();
			np2 = expr();
			np1 = array(np1, np2);
			expect(']');
			break;
		case DEC:	 case INC:
			op = (yytoken == INC) ? OPINC : OPDEC;
			/* TODO: check that the the base type is a complete type */
			np1 = unarycode(op, np1->type, np1);
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
	char op;

	switch (yytoken) {
	case INC: case DEC:
		op = (yytoken == INC) ? OINC : ODEC;
		/* TODO: check that the the base type is a complete type */
		next();
		np = unary();
		return unarycode(op, np->type, np);
		break;
	default:
		return postfix();
	}
}

Node *
expr(void)
{
	Node *np;

	do
		np = unary();
	while (yytoken == ',');
	return np;
}
