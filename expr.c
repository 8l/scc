#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

#define SWAP(x1, x2, t) (t = x1, x1 = x2, x2 = t)
#define GETBTYPE(n, tp, t) ((t) = (tp = UNQUAL(n->type))->op)

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
	Type *tp;

	if (options.npromote)
		return np;
	tp = UNQUAL(np->type);
	if (tp == chartype || tp == shortype || tp == booltype)
		return castcode(np, inttype);
	else if (tp == uchartype || tp == ushortype)
		return castcode(np, uinttype);
	return np;
}

static void
typeconv(Node **p1, Node **p2)
{
	Type *tp1, *tp2, *new1, *new2;
	Node *np1 = *p1, *np2 = *p2;
	signed char n;

	tp1 = UNQUAL(np1->type);
	tp2 = UNQUAL(np2->type);
	new1 = new2 = NULL;
	n = tp1->u.size - tp2->u.size;

	if (n > 0)
		new2 = tp1;
	else if (n < 0)
		new1 = tp2;
	else if (tp1->sign)
		new2 = tp1;
	else
		new1 = tp2;

	if (new1)
		*p1 = castcode(np1, new1);
	else
		*p2 = castcode(np2, new2);
}

static Node *
bitlogic(char op, Node *np1, Node *np2)
{
	Type *tp1, *tp2;
	uint8_t t1, t2;

	np1 = promote(np1);
	np2 = promote(np2);
	GETBTYPE(np1, tp1, t1);
	GETBTYPE(np2, tp2, t2);

	if (t1 != INT || t2 != INT)
		error("No integer operand in bit logical operation");
	if (tp1 != tp2)
		typeconv(&np1, &np2);
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
Node *
convert(Node *np, Type *tp1, char iscast)
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
		case PTR:
			if (iscast || tp1 == pvoidtype ||  tp2 == pvoidtype) {
				/* TODO:
				 * we assume conversion between pointers
				 * do not need any operation, but due to
				 * alignment problems that may be false
				 */
				np->type = tp1;
				return np;
			}
			return NULL;

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

	np1 = promote(np1);
	np2 = promote(np2);
	GETBTYPE(np1, tp1, t1);
	GETBTYPE(np2, tp2, t2);

	switch (t1) {
	case INT: case FLOAT:
		switch (t2) {
		case INT: case FLOAT:
			if (tp1 != tp2)
				typeconv(&np1, &np2);
			tp1 = np1->type;
			break;
		case PTR: case ARY:
			SWAP(np1, np2, naux);
			SWAP(t1, t2, taux);
			goto pointer;
		default:
			goto incorrect;
		}
		break;
	case ARY:
		np1 = addr2ptr(np1);
		tp1 = np1->type;
	case PTR:
pointer:
		/* TODO: substraction between pointers */
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

	np1 = promote(np1);
	np2 = promote(np2);
	GETBTYPE(np1, tp1, t1);
	GETBTYPE(np2, tp2, t2);

	switch (t1) {
	case INT: case FLOAT:
		switch (t2) {
		case INT: case FLOAT:
			if (tp1 != tp2)
				typeconv(&np1, &np2);
			break;
		default:
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
exp2cond(Node *np, char neg)
{
	if (ISNODELOG(np)) {
			np->u.op ^= neg;
			return np;
	}

	return compare(ONE ^ neg, np, constcode(zero));
}

static Node *
logic(char op, Node *np1, Node *np2)
{
	np1 = exp2cond(np1, 0);
	np2 = exp2cond(np2, 0);
	return bincode(op, inttype, np1, np2);
}

static Node *
array(Node *np1, Node *np2)
{
	Type *tp;
	uint8_t t1, t2;
	char *err;

	t1 = BTYPE(np1->type);
	t2 = BTYPE(np2->type);
	if (t1 != INT && t2 != INT)
		goto bad_subs;
	np1 = arithmetic(OADD, np1, np2);
	tp = np1->type;
	if (tp->op != PTR)
		goto bad_vector;
	np1 =  unarycode(OARY, tp->type , np1);
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
iszero(Node *np)
{
	if (ISNODELOG(np))
		return np;
	return compare(ONE, np, constcode(zero));
}

static Node *
eval(Node *np)
{
	if (!ISNODELOG(np))
		return np;
	return ternarycode(np, constcode(one), constcode(zero));
}

static Node *
assignop(char op, Node *np1, Node *np2)
{
	char *err;

	if (!np1->b.lvalue)
		goto nolvalue;
	if (isconst(np1->type->op))
		goto const_mod;
	np2 = eval(np2);
	if ((np2 = convert(np2, np1->type, 0)) == NULL)
		goto incompatibles;
	return bincode(op, np1->type, np1, np2);

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

static Node *
incdec(Node *np, char op)
{
	Type *tp;
	uint8_t t;
	char *err;

	GETBTYPE(np, tp, t);
	if (!np->b.lvalue)
		goto nolvalue;
	if (isconst(np->type->op))
		goto const_mod;

	switch (t) {
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

/*************************************************************
 * grammar functions                                         *
 *************************************************************/
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
	/* TODO: case STRING: */
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
		/* TODO: case '.': */
		/* TODO: case INDIR: */
		/* TODO: case '(' */
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
	char op, *err;
	uint8_t t;

	switch (yytoken) {
	case SIZEOF:
		next();
		if (yytoken == '(' && ahead() == TYPE) {
			next();
			tp = typename();
			expect(')');
		} else {
			tp = unary()->type;
			/* TODO: Free memory */
		}
		return sizeofcode(tp);
	case INC: case DEC:
		op = (yytoken == INC) ? OINC : ODEC;
		next();
		return incdec(unary(), op);
	case '!': case '&': case '*': case '+': case '~': case '-':
		op = yytoken;
		next();
		np = cast();
		tp = UNQUAL(np->type);
		t = tp->op;
		switch (op) {
		case '*':
			op = OPTR;
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
		case '&':
			op = OADDR;
			if (!np->b.lvalue)
				goto no_lvalue;
			if (ISNODESYM(np) && np->u.sym->s.isregister)
				goto reg_address;
			tp = mktype(tp, PTR, NULL, 0);
			break;
		case '!':
			switch (t) {
			case FTN: case ARY:
				np = addr2ptr(np);
			case INT: case FLOAT: case PTR:
				return exp2cond(np, 1);
				break;
			default:
				goto bad_operand;
			}
		case '+':
			if (t != INT)
				goto bad_operand;
			return np;
		case '~':
			op = OCPL;
			if (t != INT)
				goto bad_operand;
			break;
		case '-':
			op = ONEG;
			if (t != INT && t != FLOAT)
				goto bad_operand;
		}
		return unarycode(op, tp, np);
	default: return postfix();
	}

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
			if ((np2 = convert(np1,  tp, 1)) == NULL)
				error("bad type convertion requested");
			np2->b.lvalue = np1->b.lvalue;
			return np2;
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
		case '%': op = OMOD; break; /* TODO: check int type */
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
	Node *np, *ifyes, *ifno;
	Type *tp1, *tp2;

	np = or();
	while (accept('?')) {
		ifyes = promote(expr());
		expect(':');
		ifno = promote(ternary());
		tp1 = UNQUAL(ifyes->type);
		tp2 = UNQUAL(ifno->type);
		if (tp1 != tp2)
			typeconv(&ifyes, &ifno);
		np = ternarycode(iszero(np), ifyes, ifno);
	}
	return np;
}

static Node *
assign(void)
{
	register Node *np;

	np = ternary();
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
		default: return np;
		}
		next();
		np = assignop(op, np, assign());
	}
}

Node *
expr(void)
{
	register Node *np1, *np2;

	np1 = assign();
	while (accept(',')) {
		np2 = assign();
		np1 = bincode(OCOMMA, np2->type, np1, np2);
	}

	return np1;
}
