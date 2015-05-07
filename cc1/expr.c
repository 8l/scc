#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

#define BTYPE(np) ((np)->type->op)
#define TYPE(tp) node(OTYP, (tp), NULL, NULL)

extern Symbol *zero, *one;

Node *expr(void);

/* TODO: Change np1 and np2 to left and right (or l, r) */

static Node *
promote(Node *np)
{
	Type *tp;
	uint8_t r;
	extern uint8_t npromote;

	if (npromote)
		return np;
	tp = np->type;
	r = tp->n.rank;
	if  (r > RANK_UINT || tp == inttype || tp == uinttype)
		return np;
	tp = (r == RANK_UINT) ? uinttype : inttype;
	return node(OCAST, tp, np, NULL);
}

static void
typeconv(Node **p1, Node **p2)
{
	Type *tp1, *tp2;
	Node *np1, *np2;
	int8_t n;

	np1 = promote(*p1);
	np2 = promote(*p2);

	tp1 = np1->type;
	tp2 = np2->type;
	if (tp1 != tp2) {
		if ((n = tp1->n.rank - tp2->n.rank) > 0)
			np2 = node(OCAST, tp1, np2, NULL);
		else if (n < 0)
			np1 = node(OCAST, tp2, np1, NULL);
	}
	*p1 = np1;
	*p2 = np2;
}

static void
chklvalue(Node *np, Type *tp)
{
	if (!np->lvalue)
		error("lvalue required in operation");
	if (np->type == voidtype)
		error("invalid use of void expression");
}

Node *
eval(Node *np)
{
	Node *p;

	if (!np)
		return NULL;
	if (np->op != OAND && np->op != OOR)
		return np;
	p = node(0, inttype, symbol(one), symbol(zero));
	return node(OASK, inttype, np, p);
}

static Node *
integerop(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	if (BTYPE(np1) != INT || BTYPE(np2) != INT)
		error("operator requires integer operands");
	typeconv(&np1, &np2);
	return node(op, np1->type, np1, np2);
}

static Node *
numericaluop(char op, Node *np)
{
	np = eval(np);
	switch (BTYPE(np)) {
	case INT: case FLOAT:
		if (op == OADD)
			return np;
		return node(op, np->type, np, NULL);
	default:
		error("unary operator requires integer operand");
	}
}

static Node *
integeruop(char op, Node *np)
{
	np = eval(np);
	if (BTYPE(np) != INT)
		error("unary operator requires integer operand");
	return node(op, np->type, np, NULL);
}

static Node *
decay(Node *np)
{
	return node(OADDR, mktype(np->type, PTR, 0, NULL), np, NULL);
}

/*
 * Convert a Node to a type
 */
Node *
convert(Node *np, Type *tp, char iscast)
{
	if (eqtype(np->type, tp))
		return np;
	switch (BTYPE(np)) {
	case ENUM: case INT: case FLOAT:
		switch (tp->op) {
		case PTR:
			if (!iscast || BTYPE(np) == FLOAT)
				return NULL;
			/* PASSTHROUGH */
		case INT: case FLOAT: case ENUM: case VOID:
			break;
		default:
			return NULL;
		}
		break;
	case PTR:
		switch (tp->op) {
		case ENUM: case INT: case VOID: /* TODO: allow p = 0 */
			if (!iscast)
				return NULL;;
			break;
		case PTR:
			if (iscast ||
			    tp == pvoidtype ||
			    np->type == pvoidtype) {
				/* TODO:
				 * we assume conversion between pointers
				 * do not need any operation, but due to
				 * alignment problems that may be false
				 */
				np->type = tp;
				return np;
			}
		default:
			return NULL;
		}
	default:
			return NULL;
	}
	return node(OCAST, tp, np, NULL);
}

static Node *
parithmetic(char op, Node *np1, Node *np2)
{
	Type *tp;
	Node *size;

	tp = np1->type;
	size = node(OSIZE, inttype, TYPE(tp->type), NULL);
	if (BTYPE(np2) == ARY)
		np2 = decay(np2);

	if (op == OSUB && BTYPE(np2) == PTR) {
		if (tp != np2->type)
			goto incorrect;
		np1 = node(OSUB, inttype, np1, np2);
		return node(ODIV, inttype, np1, size);
	}
	if (BTYPE(np2) != INT)
		goto incorrect;
	np2 = node(OCAST, tp, promote(np2), NULL);
	np2 = node(OMUL, tp, np2, size);
	return node(op, tp, np1, np2);

incorrect:
	error("incorrect arithmetic operands");
}

static Node *
arithmetic(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	switch (BTYPE(np1)) {
	case INT: case FLOAT:
		switch (BTYPE(np2)) {
		case INT: case FLOAT:
			typeconv(&np1, &np2);
			break;
		case ARY:
			np2 = decay(np2);
		case PTR:
			if (op == OADD || op == OSUB)
				return parithmetic(op, np2, np1);
		default:
			goto incorrect;
		}
		break;
	case ARY:
		np1 = decay(np1);
	case PTR:
		return parithmetic(op, np1, np2);
	default:
	incorrect:
		error("incorrect arithmetic operands");
	}

	return node(op, np1->type, np1, np2);
}

static Node *
pcompare(char op, Node *np1, Node *np2)
{
	switch (BTYPE(np2)) {
	case INT:
		if (np2->symbol && np2->sym->u.i == 0)
			np2 = node(OCAST, pvoidtype, np2, NULL);
		break;
	case PTR:
		if (np1->type != np2->type)
			warn("comparision between different pointer types");
		break;
	default:
		error("incompatibles type in comparision");
	}

	return node(op, np1->type, np1, np2);
}

static Node *
compare(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	switch (BTYPE(np1)) {
	case INT: case FLOAT:
		switch (BTYPE(np1)) {
		case INT: case FLOAT:
			typeconv(&np1, &np2);
			break;
		case ARY: case FTN:
			np2 = decay(np2);
		case PTR:
			return pcompare(op, np2, np1);
		default:
			goto nocompat;
		}
		break;
	case ARY: case FTN:
		np1 = decay(np1);
	case PTR:
		return pcompare(op, np1, np2);
	default:
	nocompat:
		error("incompatibles type in comparision");
	}

	return node(op, inttype, np1, np2);
}

Node *
negate(Node *np)
{
	uint8_t op;

	switch (np->op) {
	case OAND: op = OOR;  break;
	case OOR:  op = OAND; break;
	case OEQ:  op = ONE;  break;
	case ONE:  op = OEQ;  break;
	case OLT:  op = OGE;  break;
	case OGE:  op = OLT;  break;
	case OLE:  op = OGT;  break;
	case OGT:  op = OLE;  break;
	default:
		abort();
	}
	np->op = op;
	return np;
}

static bool
isnodecmp(Node *np)
{
	switch (np->op) {
	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OLE:
	case OGT:
		return 1;
	default:
		return 0;
	}
}

static Node *
exp2cond(Node *np, char neg)
{
	if (isnodecmp(np))
		return (neg) ? negate(np) : np;
	return compare(ONE ^ neg, np, symbol(zero));
}

static Node *
logic(char op, Node *np1, Node *np2)
{
	np1 = exp2cond(np1, 0);
	np2 = exp2cond(np2, 0);
	return node(op, inttype, np1, np2);
}

static Node *
field(Node *np)
{
	extern uint8_t lex_ns;
	Symbol *sym;

	switch (BTYPE(np)) {
	case STRUCT: case UNION:
		lex_ns = np->type->ns;
		next();
		if (yytoken != IDEN)
			unexpected();
		if ((sym = yylval.sym) == NULL)
			error("incorrect field in struct/union");
		lex_ns = NS_IDEN;
		next();
		return node(OFIELD, sym->type, symbol(sym), np);
	default:
		error("struct or union expected");
	}
}

static Node *
array(Node *np1, Node *np2)
{
	Type *tp;

	if (BTYPE(np1) != INT && BTYPE(np2) != INT)
		error("array subscript is not an integer");
	np1 = arithmetic(OADD, np1, np2);
	tp = np1->type;
	if (tp->op != PTR)
		error("subscripted value is neither array nor pointer nor vector");
	np1 =  node(OPTR, tp->type, np1, NULL);
	np1->lvalue = 1;
	return np1;
}

Node *
iszero(Node *np)
{
	if (isnodecmp(np))
		return np;
	return compare(ONE, np, symbol(zero));
}

static Node *
assignop(char op, Node *np1, Node *np2)
{
	switch (np2->type->op) {
	case FTN: case ARY:
		np2 = decay(np2);
		/* PASSTHROUGH */
	default:
		if ((np2 = convert(np2, np1->type, 0)) == NULL)
			error("incompatible types when assigning");
	}
	return node(op, np1->type, np1, np2);
}

static Node *
incdec(Node *np, char op)
{
	Type *tp = np->type;
	Node *inc;

	chklvalue(np, np->type);

	switch (BTYPE(np)) {
	case PTR:
		if (!tp->defined)
			error("invalid use of indefined type");
		inc = node(OSIZE, inttype, TYPE(tp->type), NULL);
		break;
	case INT: case FLOAT:
		inc = symbol(one);
		break;
	default:
		error("incorrect type in arithmetic operation");
	}
	return arithmetic(op, np, inc);
}

static Node *
address(char op, Node *np)
{
	if (!np->lvalue)
		error("lvalue required in unary expression");
	if (np->symbol && np->sym->isregister)
		error("address of register variable '%s' requested", yytext);
	return node(op, mktype(np->type, PTR, 0, NULL), np, NULL);
}

static Node *
content(char op, Node *np)
{
	switch (BTYPE(np)) {
	case ARY: case FTN:
		np = decay(np);
	case PTR:
		np = node(op, np->type->type, np, NULL);
		np->lvalue = 1;
		return np;
	default:
		error("invalid argument of unary '*'");
	}
}

static Node *
negation(char op, Node *np)
{
	switch (BTYPE(np)) {
	case FTN: case ARY:
		np = decay(np);
	case INT: case FLOAT: case PTR:
		return exp2cond(np, 1);
	default:
		error("invalid argument of unary '!'");
	}
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
	case STRING: case CONSTANT: case IDEN:
		if ((sym = yylval.sym) == NULL)
			error("'%s' undeclared", yytext);
		np = symbol(yylval.sym);
		if (yytoken == IDEN) {
			np->lvalue = 1;
			np->constant = 0;
		}
		next();
		break;
	case '(':
		next();
		np = expr();
		expect(')');
		break;
	default:
		unexpected();
	}
	return np;
}

static Node *assign(void);

static Node *
arguments(Node *np)
{
	Node *par;

	/* TODO: Check type of np */
	expect('(');
	if (accept(')'))
		return np;

	do {
		if ((par = eval(assign())) == NULL)
			unexpected();
	} while (accept(','));

	expect(')');
	return np;
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
			np1 = incdec(np1, (yytoken == INC) ? OINC : ODEC);
			next();
			break;
		case INDIR:
			np1 = content(OPTR, np1);
		case '.':
			np1 = field(np1);
			break;
		case '(':
			np1 = arguments(np1);
			break;
		default:
			return np1;
		}
	}
}

static Node *unary(void);

static Type *
typeof(Node *np)
{
	Type *tp;

	if (np == NULL)
		unexpected();
	tp = np->type;
	/* TODO: free np */
	return tp;
}

static Type *
sizeexp(void)
{
	Type *tp;

	expect('(');
	switch (yytoken) {
	case TYPE: case TYPEIDEN:
		tp = typename();
		break;
	default:
		tp = typeof(unary());
		break;
	}
	expect(')');
	return tp;
}

static Node *cast(void);

static Node *
unary(void)
{
	Node *(*fun)(char, Node *);
	char op;
	Type *tp;

	switch (yytoken) {
	case SIZEOF:
		next();
		tp = (yytoken == '(') ? sizeexp() : typeof(unary());
		return node(OSIZE, inttype, TYPE(tp), NULL);
	case INC: case DEC:
		op = (yytoken == INC) ? OA_ADD : OA_SUB;
		next();
		return incdec(unary(), op);
	case '!': op = 0;     fun = negation;     break;
	case '+': op = OADD;  fun = numericaluop; break;
	case '-': op = ONEG;  fun = numericaluop; break;
	case '~': op = OCPL;  fun = integeruop;   break;
	case '&': op = OADDR; fun = address;      break;
	case '*': op = OPTR;  fun = content;      break;
	default:  return postfix();
	}

	next();
	return (*fun)(op, cast());
}

static Node *
cast(void)
{
	Node *np1, *np2;
	Type *tp;

	if (!accept('('))
		return unary();

	switch (yytoken) {
	case TQUALIFIER: case TYPE:
		tp = typename();
		switch (tp->op) {
		case ARY:
			error("cast specify an array type");
		case FTN:
			error("cast specify a function type");
		default:
			expect(')');
			if ((np1 = eval(cast())) == NULL)
				unexpected();
			if ((np2 = convert(np1,  tp, 1)) == NULL)
				error("bad type convertion requested");
			np2->lvalue = np1->lvalue;
		}
		break;
	default:
		np2 = expr();
		expect(')');
		break;
	}

	return np2;
}

static Node *
mul(void)
{
	Node *np, *(*fun)(char, Node *, Node *);
	char op;

	np = cast();
	for (;;) {
		switch (yytoken) {
		case '*': op = OMUL; fun = arithmetic; break;
		case '/': op = ODIV; fun = arithmetic; break;
		case '%': op = OMOD; fun = integerop;  break;
		default: return np;
		}
		next();
		np = (*fun)(op, np, cast());
	}
}

static Node *
add(void)
{
	char op;
	Node *np;

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
	char op;
	Node *np;

	np = add();
	for (;;) {
		switch (yytoken) {
		case SHL: op = OSHL; break;
		case SHR: op = OSHR; break;
		default:  return np;
		}
		next();
		np = integerop(op, np, add());
	}
}

static Node *
relational(void)
{
	char op;
	Node *np;

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
	char op;
	Node *np;

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
	Node *np;

	np = eq();
	while (accept('&'))
		np = integerop(OBAND, np, eq());
	return np;
}

static Node *
bit_xor(void)
{
	Node *np;

	np = bit_and();
	while (accept('^'))
		np = integerop(OBXOR,  np, bit_and());
	return np;
}

static Node *
bit_or(void)
{
	Node *np;

	np = bit_xor();
	while (accept('|'))
		np = integerop(OBOR, np, bit_xor());
	return np;
}

static Node *
and(void)
{
	Node *np;

	np = bit_or();
	while (accept(AND))
		np = logic(OAND, np, bit_or());
	return np;
}

static Node *
or(void)
{
	Node *np;

	np = and();
	while (accept(OR))
		np = logic(OOR, np, and());
	return np;
}

static Node *
ternary(void)
{
	Node *cond;

	cond = or();
	while (accept('?')) {
		Node *ifyes, *ifno, *np;

		cond = exp2cond(cond, 0);
		ifyes = promote(expr());
		expect(':');
		ifno = promote(ternary());
		typeconv(&ifyes, &ifno);
		np = node(0, ifyes->type, ifyes, ifno);
		cond = node(OASK, np->type, cond, np);
	}
	return cond;
}

static Node *
assign(void)
{
	Node *np, *(*fun)(char , Node *, Node *);
	char op;

	np = ternary();
	for (;;) {
		switch (yytoken) {
		case '=':    op = OASSIGN; fun = assignop;   break;
		case MUL_EQ: op = OA_MUL;  fun = arithmetic; break;
		case DIV_EQ: op = OA_DIV;  fun = arithmetic; break;
		case MOD_EQ: op = OA_MOD;  fun = integerop;  break;
		case ADD_EQ: op = OA_ADD;  fun = arithmetic; break;
		case SUB_EQ: op = OA_SUB;  fun = arithmetic; break;
		case SHL_EQ: op = OA_SHL;  fun = integerop;  break;
		case SHR_EQ: op = OA_SHR;  fun = integerop;  break;
		case AND_EQ: op = OA_AND;  fun = integerop;  break;
		case XOR_EQ: op = OA_XOR;  fun = integerop;  break;
		case OR_EQ:  op = OA_OR;   fun = integerop;  break;
		default: return np;
		}
		chklvalue(np, np->type);
		next();
		np = (fun)(op, np, eval(assign()));
	}
}

Node *
expr(void)
{
	Node *np1, *np2;

	np1 = assign();
	while (accept(',')) {
		np2 = assign();
		np1 = node(OCOMMA, np2->type, np1, np2);
	}

	return np1;
}
