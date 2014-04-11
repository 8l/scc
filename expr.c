#include <stdint.h>
#include <stdio.h>

#include "cc.h"

#define SWAP(x1, x2, t) (t = x1, x1 = x2, x2 = t)

Node *expr(void);

static Node *
promote(Node *np)
{
}

static void
intconv(Node **np1, Node **np2)
{
}

static void
floatconv(Node **np1, Node **np2)
{
}

static Node *
bitlogic(char op, Node *np1, Node *np2)
{
	Type *tp1, *tp2;
	uint8_t t1, t2;

	tp1 = UNQUAL(np1->type), tp2 = UNQUAL(np2->type);
	t1 = tp1->op, t2 = tp2->op;

	if (t1 != INT || t2 != INT)
		error("No integer operand in bit logical operation");
	intconv(&np1, &np2);
	return bincode(op, np1->type, np1, np2);
}

static Node *
arithmetic(char op, Node *np1, Node *np2)
{
	char *err;
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
		switch (op) {
		case OLT: case OGT: case OGE:
		case OLE:	case OEQ: case ONE:
			if (t1 == ARY)
				tp1 = mktype(tp1->type, PTR, NULL, 0);
			else if (t1 != PTR)
				goto incorrect;
			if (t2 == ARY)
				tp2 = mktype(tp2->type, PTR, NULL, 0);
			else if (t2 != PTR)
				goto incorrect;
			/* FIX: result of comparisions is integer type */
			break;
		case OADD: OSUB:
			tp3 = tp1->type; /* TODO: I think tp3 is not needed */
			if (!tp1->defined)
				goto nocomplete;
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
		break;
	default:
		goto incorrect;
	}

	return bincode(op, tp1, np1, np2);

nocomplete:
	err = "invalid use of indefined type";
	goto error;
incorrect:
	err = "incorrect arithmetic operands";
error:
	error(err);
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
	np1 =  unarycode(OARY, np1->type->type , np1);
	np1->b.lvalue = 1;
	return np1;

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

static Node *
postfix(void)
{
	register Node *np1, *np2;

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
	register Node *np;

	switch (yytoken) {
	case INC: case DEC:
		np = incdec(unary(), (yytoken == INC) ? OINC : ODEC);
		next();
		break;
	default:
		return postfix();
	}
}

static Node *
cast(void)
{
	Type *tp;
	register Node *np1, *np2;
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

static Node *
mul(void)
{
	register Node *np;
	register char op;

	np = cast();
	for (;;) {
		switch (yytoken) {
		case '*': op = OMUL; break;
		case '/': op = ODIV; break;
		case '%': op = OMOD; break;
		default: return np;
		}
		next();
		np = arithmetic(op, np, cast());
	}
}

static Node *
add(void)
{
	register char op;
	register Node *np;

	np = mul();
	for (;;) {
		switch (yytoken) {
		case '+': op = OADD; break;
		case '-': op = OSUB; break;
		default:  return np;
		}
		next();
		np = arithmetic(op, np, mul());
	}
}

static Node *
shift(void)
{
	register char op;
	register Node *np;

	np = add();
	for (;;) {
		switch (yytoken) {
		case SHL: op = OSHL; break;
		case SHR: op = OSHR; break;
		default:  return np;
		}
		next();
		np = bitlogic(op, np, add());
	}
}

static Node *
relational(void)
{
	register char op;
	register Node *np;

	np = shift();
	for (;;) {
		switch (yytoken) {
		case '<': op = OLT; break;
		case '>': op = OGT; break;
		case GE:  op = OGE; break;
		case LE:  op = OLE; break;
		default:  return np;
		}
		next();
		/*
		 * FIX: This is incorrect because in relation
		 * we cannot change the order of the operands
		 */
		np = arithmetic(op, np, shift());
	}
}

static Node *
eq(void)
{
	register char op;
	register Node *np;

	np = relational();
	for (;;) {
		switch (yytoken) {
		case EQ: op = OEQ; break;
		case NE: op = ONE; break;
		default: return np;
		}
		next();
		np = arithmetic(op, np, relational());
	}
}

static Node *
bit_and(void)
{
	register Node *np;

	np = eq();
	while (yytoken == '&') {
		next();
		np = bitlogic(OBAND, np, eq());
	}
	return np;
}

static Node *
bit_xor(void)
{
	register Node *np;

	np = bit_and();
	while (yytoken == '^') {
		next();
		np = bitlogic(OBXOR,  np, bit_and());
	}
	return np;
}

static Node *
bit_or(void)
{
	register Node *np;

	np = bit_xor();
	while (yytoken == '|') {
		next();
		np = bitlogic(OBOR, np, bit_xor());
	}
	return np;
}

static Node *
assign(void)
{
	register Node *np = bit_or();

	for (;;) {
		register char op;

		switch (yytoken) {
		case '=':    op = OASSIGN; break;
		case MUL_EQ: op = OA_MUL;  break;
		case DIV_EQ: op = OA_DIV;  break;
		case MOD_EQ: op = OA_MOD;  break;
		case ADD_EQ: op = OA_ADD;  break;
		case SUB_EQ: op = OA_SUB;  break;
		case SHL_EQ: op = OA_SHL;  break;
		case SHR_EQ: op = OA_SHR;  break;
		case AND_EQ: op = OA_AND;  break;
		case XOR_EQ: op = OA_XOR;  break;
		case OR_EQ:  op = OA_OR;   break;
		default:  goto return_np;
		}
		next();
		/* TODO: cast types */
		np = bincode(op, np->type, np, assign());
	}
return_np:
	return np;
}

Node *
expr(void)
{
	register Node *np;

	do
		np = assign();
	while (yytoken == ',');
	return np;
}
