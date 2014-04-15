#include <stdint.h>
#include <stdio.h>

#include "cc.h"

#define SWAP(x1, x2, t) (t = x1, x1 = x2, x2 = t)

static Symbol *zero, *one;

Node *expr(void);

void
init_expr(void)
{
	static Symbol zdummy, odummy;

	zdummy.type = odummy.type = inttype;
	zdummy.u.i = 0;
	odummy.u.i = 1;
	zero = &zdummy;
	one = &odummy;
}

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
logic(char op, Node *np1, Node *np2)
{
	return np1;
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
addr2ptr(Node *np)
{
	Type *tp;

	tp = UNQUAL(np->type);
	tp = mktype(tp->type, PTR, NULL, 0);
	return unarycode(OADDR, tp, np);
}

/*
 * Convert a Node to a type
 */
static Node *
convert(Node *np, Type *tp1)
{
	Type *tp2;
	register uint8_t t1, t2;

	tp1 = UNQUAL(tp1), tp2 = UNQUAL(np->type);
	if (tp1 == tp2)
		return np;
	t1 = tp1->op, t2 = tp2->op;

	switch (t1) {
	case ENUM: case INT: case FLOAT:
		switch (t2) {
		case INT: case FLOAT: case ENUM:
			return castcode(np, tp1);
		}
		break;
	case PTR:
		switch (t2) {
		case ARY: case FTN:
			np = addr2ptr(np);
			np->type = tp1;
			return np;
		}
	}
	return NULL;
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
			np2 = castcode(np1, np2->type);
			break;
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
			np2 = castcode(np2, np1->type);
			break;
		default:
			goto incorrect;
		}
		break;
	case ARY:
		np1 = addr2ptr(np1);
		tp1 = np1->type;
	case PTR:
pointer:
		switch (op) {
		case OADD: case OSUB:
			tp3 = tp1->type;
			if (!tp3->defined)
				goto nocomplete;
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
compare(char op, Node *np1, Node *np2)
{
	Type *tp1, *tp2;
	uint8_t t1, t2;

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
			np1 = castcode(np1, tp2);
			break;
		default:
			goto incompatibles;
		}
		break;
	case FLOAT:
		switch (t2) {
		case INT:
			np2 = castcode(np2, tp1);
			break;
		case FLOAT:
			if (tp1 != tp2)
				floatconv(&np1, &np2);
			break;
		defualt:
			goto incompatibles;
		}
		break;
	case ARY: case FTN:
		np1 = addr2ptr(np1);
		tp1 = UNQUAL(np1->type);
	case PTR:
		if (tp1 != tp2)
			goto incompatibles;
		break;
	default:
		goto incompatibles;
	}

	return bincode(op, inttype, np1, np2);

incompatibles:
	error("incompatibles type in comparision");
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
eval(Node *np, char neg)
{
	Node *ifyes, *ifno, *cmp;
	char op;

	ifyes = constcode(one);
	ifno = constcode(zero);
	cmp = constcode(zero);
	op = (neg) ?  OEQ : ONE;

	return ternarycode(compare(op, np, cmp), ifyes, ifno);
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
		np = constcode(yylval.sym);
		next();
		break;
	case '(':
		next();
		np = expr();
		expect(')');
	case ';':
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

static Node *cast(void);

static Node *
unary(void)
{
	register Node *np;
	Type *tp;
	char paren, op, *err;
	uint8_t t;

	switch (yytoken) {
	case SIZEOF:
		next();
		if (accept('('))
			paren = 1;
		switch (yytoken) {
		case TQUALIFIER: case TYPE:
			tp = typename();
			break;
		default:
			tp = expr()->type;
			/* TODO: Free memory */
			break;
		}
		if (paren)
			expect(')');
		return sizeofcode(tp);
	case INC: case DEC:
		op = (yytoken == INC) ? OINC : ODEC;
		next();
		return incdec(unary(), op); /* TODO: unary or cast? */
	case '!': op = OEXC; break;
	case '&': op = OADDR; break;
	case '*': op = OPTR; break;
	case '+': op = OADD; break;
	case '~': op = OCPL; break;
	case '-':  op = ONEG; break;
	default: return postfix();
	}

	next();
	np = cast();
	tp = UNQUAL(np->type);
	t = tp->op;

	switch (op) {
	case OPTR:
		switch (t) {
		case ARY: case FTN:
			np = addr2ptr(np);
		case PTR:
			tp = tp->type;
			break;
		default:
			goto bad_operand;
		}
		break;
	case OADDR:
		if (!np->b.lvalue)
			goto no_lvalue;
		if (np->code == emitsym && np->u.sym->s.isregister)
			goto reg_address;
		tp = mktype(tp, PTR, NULL, 0);
		break;
	case OEXC:
		switch (t) {
		case FTN: case ARY:
			np = addr2ptr(np);
		case INT: case FLOAT: case PTR:
			return eval(np, 1);
			break;
		default:
			goto bad_operand;
		}
	case OADD:
		if (t != INT)
			goto bad_operand;
		return np;
	case OCPL:
		if (t != INT)
			goto bad_operand;
	case ONEG:
		if (!isarith(t))
			goto bad_operand;
	}

	return unarycode(op, tp, np);

no_lvalue:
	err = "lvalue required in unary expression";
	goto error;
reg_address:
	err = "address of register variable '%s' requested";
	goto error;
bad_operand:
	err = "bad operand in unary expression";
error:
	error(err, yytext);
}

static Node *
cast(void)
{
	Type *tp;
	register Node *np1, *np2;

	if (yytoken == '(') {
		switch(ahead()) {
		case TQUALIFIER: case TYPE:
			next();
			tp = typename();
			expect(')');
			np1 = cast();
			if ((np2 = convert(np1,  tp)) == NULL)
				error("bad type convertion requested");
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
		np = compare(op, np, shift());
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
		np = compare(op, np, relational());
	}
}

static Node *
bit_and(void)
{
	register Node *np;

	np = eq();
	while (accept('&'))
		np = bitlogic(OBAND, np, eq());
	return np;
}

static Node *
bit_xor(void)
{
	register Node *np;

	np = bit_and();
	while (accept('^'))
		np = bitlogic(OBXOR,  np, bit_and());
	return np;
}

static Node *
bit_or(void)
{
	register Node *np;

	np = bit_xor();
	while (accept('|'))
		np = bitlogic(OBOR, np, bit_xor());
	return np;
}

static Node *
and(void)
{
	register Node *np;

	np = bit_or();
	while (accept(AND))
		np = logic(OAND, np, bit_or());
	return np;
}

static Node *
or(void)
{
	register Node *np;

	np = and();
	while (accept(OR))
		np = logic(OOR, np, and());
	return np;
}

static Node *
ternary(void)
{
	register Node *cond, *ifyes, *ifno;

	cond = bit_or();
	while (accept('?')) {
		ifyes = expr();
		expect(':');
		ifno = expr();
		/* TODO: check the types of ifno and ifyes */
		cond = ternarycode(cond, ifyes, ifno);
	}
	return cond;
}

static Node *
assign(void)
{
	register Node *np1, *np2;
	char *err;

	np1 = ternary();
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
		np2 = assign();
		if (!np1->b.lvalue)
			goto nolvalue;
		if (isconst(np1->type->op))
			goto const_mod;
		/* TODO: if it necessary a 0 comparision? */
		if ((np2 = convert(np2, np1->type)) == NULL)
			goto incompatibles;
		np1 = bincode(op, np1->type, np1, np2);
	}
return_np:
	return np1;

const_mod:
	err = "const value modified";
	goto error;
nolvalue:
	err = "lvalue required as left operand of assignment";
	goto error;
incompatibles:
	err = "incompatible types when assigning";
error:
	error(err);
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
