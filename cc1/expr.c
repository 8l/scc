#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

#define BTYPE(np) ((np)->type->op)

extern Symbol *zero, *one;

Node *expr(void);

static Node *
promote(Node *np)
{
	Type *tp;
	unsigned r;

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
	int n;

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
	p = node(OCOLON, inttype, constnode(one), constnode(zero));
	return node(OASK, inttype, np, p);
}

static Node *
integerop(char op, Node *lp, Node *rp)
{
	lp = eval(lp);
	rp = eval(rp);
	if (BTYPE(lp) != INT || BTYPE(rp) != INT)
		error("operator requires integer operands");
	typeconv(&lp, &rp);
	return node(op, lp->type, lp, rp);
}

static Node *
numericaluop(char op, Node *np)
{
	np = eval(np);
	switch (BTYPE(np)) {
	case INT:
	case FLOAT:
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
	Type *tp = np->type;

	if (tp->op == ARY)
		tp = tp->type;

	return node(OADDR, mktype(tp, PTR, 0, NULL), np, NULL);
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
parithmetic(char op, Node *lp, Node *rp)
{
	Type *tp;
	Node *size;

	tp = lp->type;
	size = sizeofnode(tp->type);
	if (BTYPE(rp) == ARY)
		rp = decay(rp);

	if (op == OSUB && BTYPE(rp) == PTR) {
		if (tp != rp->type)
			goto incorrect;
		lp = node(OSUB, inttype, lp, rp);
		return node(ODIV, inttype, lp, size);
	}
	if (BTYPE(rp) != INT)
		goto incorrect;
	rp = node(OCAST, tp, promote(rp), NULL);
	rp = node(OMUL, tp, rp, size);
	return node(op, tp, lp, rp);

incorrect:
	error("incorrect arithmetic operands");
}

static Node *
arithmetic(char op, Node *lp, Node *rp)
{
	lp = eval(lp);
	rp = eval(rp);
	switch (BTYPE(lp)) {
	case INT:
	case FLOAT:
		switch (BTYPE(rp)) {
		case INT:
		case FLOAT:
			typeconv(&lp, &rp);
			break;
		case ARY:
			rp = decay(rp);
		case PTR:
			if (op == OADD || op == OSUB)
				return parithmetic(op, rp, lp);
		default:
			goto incorrect;
		}
		break;
	case ARY:
		lp = decay(lp);
	case PTR:
		return parithmetic(op, lp, rp);
	default:
	incorrect:
		error("incorrect arithmetic operands");
	}

	return node(op, lp->type, lp, rp);
}

static Node *
pcompare(char op, Node *lp, Node *rp)
{
	switch (BTYPE(rp)) {
	case INT:
		if (rp->symbol && rp->sym->u.i == 0)
			rp = node(OCAST, pvoidtype, rp, NULL);
		break;
	case PTR:
		if (lp->type != rp->type)
			warn("comparision between different pointer types");
		break;
	default:
		error("incompatibles type in comparision");
	}

	return node(op, lp->type, lp, rp);
}

static Node *
compare(char op, Node *lp, Node *rp)
{
	lp = eval(lp);
	rp = eval(rp);
	switch (BTYPE(lp)) {
	case INT: case FLOAT:
		switch (BTYPE(lp)) {
		case INT:
		case FLOAT:
			typeconv(&lp, &rp);
			break;
		case ARY:
		case FTN:
			rp = decay(rp);
		case PTR:
			return pcompare(op, rp, lp);
		default:
			goto nocompat;
		}
		break;
	case ARY: case FTN:
		lp = decay(lp);
	case PTR:
		return pcompare(op, lp, rp);
	default:
	nocompat:
		error("incompatibles type in comparision");
	}

	return node(op, inttype, lp, rp);
}

Node *
negate(Node *np)
{
	unsigned op;

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
	return compare(ONE ^ neg, np, constnode(zero));
}

static Node *
logic(char op, Node *lp, Node *rp)
{
	lp = exp2cond(lp, 0);
	rp = exp2cond(rp, 0);
	return node(op, inttype, lp, rp);
}

static Node *
field(Node *np)
{
	Symbol *sym;

	switch (BTYPE(np)) {
	case STRUCT: case UNION:
		setnamespace(np->type->ns);
		next();
		if (yytoken != IDEN)
			unexpected();
		if ((sym = yylval.sym) == NULL)
			error("incorrect field in struct/union");
		next();
		return node(OFIELD, sym->type, varnode(sym), np);
	default:
		error("struct or union expected");
	}
}

static Node *
array(Node *lp, Node *rp)
{
	Type *tp;

	if (BTYPE(lp) != INT && BTYPE(rp) != INT)
		error("array subscript is not an integer");
	lp = arithmetic(OADD, lp, rp);
	tp = lp->type;
	if (tp->op != PTR)
		error("subscripted value is neither array nor pointer nor vector");
	lp =  node(OPTR, tp->type, lp, NULL);
	lp->lvalue = 1;
	return lp;
}

Node *
iszero(Node *np)
{
	if (isnodecmp(np))
		return np;
	return compare(ONE, np, constnode(zero));
}

static Node *
assignop(char op, Node *lp, Node *rp)
{
	switch (rp->type->op) {
	case FTN:
	case ARY:
		rp = decay(rp);
		/* PASSTHROUGH */
	default:
		if ((rp = convert(rp, lp->type, 0)) == NULL)
			error("incompatible types when assigning");
	}
	return node(op, lp->type, lp, rp);
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
		inc = sizeofnode(tp->type);
		break;
	case INT:
	case FLOAT:
		inc = constnode(one);
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
	if (np->symbol && (np->sym->flags & ISREGISTER))
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
	case FTN:
	case ARY:
		np = decay(np);
	case INT:
	case FLOAT:
	case PTR:
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

	switch (yytoken) {
	case CONSTANT:
		np = constnode(yylval.sym);
		next();
		break;
	case IDEN:
		if (!(yylval.sym->flags & ISDEFINED)) {
			yylval.sym->type = inttype;
			yylval.sym->flags |= ISDEFINED;
			error("'%s' undeclared", yytext);
		}
		yylval.sym->flags |= ISUSED;
		np = varnode(yylval.sym);
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
	Type *tp = np->type;

	if (tp->op == PTR && tp->type->op == FTN) {
		np = content(OPTR, np);
		tp = np->type;
	}
	if (tp->op != FTN)
		error("function or function pointer expected");
	/* TODO: implement function calls */
	abort();
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
	Node *lp, *rp;

	lp = primary();
	for (;;) {
		switch (yytoken) {
		case '[':
			next();
			rp = expr();
			lp = array(lp, rp);
			expect(']');
			break;
		case DEC:
		case INC:
			lp = incdec(lp, (yytoken == INC) ? OINC : ODEC);
			next();
			break;
		case INDIR:
			lp = content(OPTR, lp);
		case '.':
			lp = field(lp);
			break;
		case '(':
			lp = arguments(lp);
			break;
		default:
			return lp;
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
	freetree(np);
	return tp;
}

static Type *
sizeexp(void)
{
	Type *tp;

	expect('(');
	switch (yytoken) {
	case TYPE:
	case TYPEIDEN:
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
		return sizeofnode(tp);
	case INC:
	case DEC:
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
	Node *lp, *rp;
	Type *tp;

	if (!accept('('))
		return unary();

	switch (yytoken) {
	case TQUALIFIER:
	case TYPE:
		tp = typename();
		switch (tp->op) {
		case ARY:
			error("cast specify an array type");
		case FTN:
			error("cast specify a function type");
		default:
			expect(')');
			if ((lp = eval(cast())) == NULL)
				unexpected();
			if ((rp = convert(lp,  tp, 1)) == NULL)
				error("bad type convertion requested");
			rp->lvalue = lp->lvalue;
		}
		break;
	default:
		rp = expr();
		expect(')');
		break;
	}

	return rp;
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
		np = simplify(np);
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
		np = node(OCOLON, ifyes->type, ifyes, ifno);
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
constexpr(void)
{
	Node *np;

	np = ternary();
	if (!np->constant)
		error("constant expression required");
	return np;
}

Node *
expr(void)
{
	Node *lp, *rp;

	lp = assign();
	while (accept(',')) {
		rp = assign();
		lp = node(OCOMMA, rp->type, lp, rp);
	}

	return lp;
}
