#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

static Symbol *zero, *one;

Node *expr(void);

void
init_expr(void)
{
	static Symbol dummy0, dummy1;

	dummy0.type = dummy1.type = inttype;
	dummy0.u.i = 0;
	dummy1.u.i = 1;
	zero = &dummy0;
	one = &dummy1;
}

static Node *
promote(Node *np)
{
	Type *tp;
	uint8_t r;

	if (options.npromote)
		return np;
	tp = np->utype;
	r = tp->u.rank;
	if  (r > RANK_UINT || tp == inttype || tp == uinttype)
		return np;
	return castcode(np, (r == RANK_UINT) ? uinttype : inttype);
}

static void
typeconv(Node **p1, Node **p2)
{
	Type *tp1, *tp2;
	Node *np1, *np2;
	int8_t n;

	np1 = promote(*p1);
	np2 = promote(*p2);

	tp1 = np1->utype;
	tp2 = np2->utype;
	if (tp1 != tp2) {
		if ((n = tp1->u.rank - tp2->u.rank) > 0)
			np2 = castcode(np2, tp1);
		else if (n < 0)
			np1 = castcode(np1, tp2);
	}
	*p1 = np1;
	*p2 = np2;
}

static Node *
eval(Node *np)
{
	if (!ISNODECMP(np))
		return np;
	return ternarycode(np, symcode(one), symcode(zero));
}

static Node *
integerop(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	if (np1->typeop != INT || np2->typeop != INT)
		error("operator requires integer operands");
	typeconv(&np1, &np2);
	return bincode(op, np1->type, np1, np2);
}

static Node *
integeruop(char op, Node *np)
{
	np = eval(np);
	if (np->typeop != INT)
		error("unary operator requires integer operand");
	return unarycode(op, np->type, np);
}

static Node *
addr2ptr(Node *np)
{
	Type *tp;

	tp = mktype(np->utype->type, PTR, NULL, 0);
	return unarycode(OADDR, tp, np);
}

/*
 * Convert a Node to a type
 */
Node *
convert(Node *np, Type *tp, char iscast)
{
	Type *utp;
	uint8_t t;

	if (np->type == tp)
		return np;

	utp = UNQUAL(tp);
	t = utp->op;

	switch (np->typeop) {
	case ENUM: case INT: case FLOAT:
		switch (t) {
		case PTR:
			if (!iscast || np->typeop == FLOAT)
				return NULL;
		case INT: case FLOAT: case ENUM: case VOID:
			break;
		default:
			return NULL;
		}
		break;
	case PTR:
		switch (t) {
		case ENUM: case INT: case VOID: /* TODO: allow p = 0 */
			if (!iscast)
				return NULL;;
			break;
		case ARY: case FTN:
			np = addr2ptr(np);
		case PTR:
			if (iscast ||
			    utp == pvoidtype ||
			    np->utype == pvoidtype) {
				/* TODO:
				 * we assume conversion between pointers
				 * do not need any operation, but due to
				 * alignment problems that may be false
				 */
				np->type = tp;
				np->utype = utp;
				return np;
			}
		default:
			return NULL;
		}
	default:
			return NULL;
	}
	return castcode(np, tp);
}

static Node *
parithmetic(char op, Node *np1, Node *np2)
{
	Type *tp;
	Node *size;
	char *err;

	tp = np1->utype;
	size = sizeofcode(tp->type);
	if (np2->typeop == ARY)
		np2 = addr2ptr(np2);

	if (op == OSUB && np2->typeop == PTR) {
		if (tp != np2->utype)
			goto incorrect;
		np1 = bincode(OSUB, inttype, np1, np2);
		return bincode(ODIV, inttype, np1, size);
	}
	if (np2->typeop != INT)
		goto incorrect;
	np2 = castcode(promote(np2), tp);
	np2 = bincode(OMUL, tp, np2, size);
	return bincode(op, tp, np1, np2);

incorrect:
	error("incorrect arithmetic operands");
}

static Node *
arithmetic(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	switch (np1->typeop) {
	case INT: case FLOAT:
		switch (np2->typeop) {
		case INT: case FLOAT:
			typeconv(&np1, &np2);
			break;
		case ARY:
			np2 = addr2ptr(np2);
		case PTR:
			if (op == OADD || op == OSUB)
				return parithmetic(op, np2, np1);
		default:
			goto incorrect;
		}
		break;
	case ARY:
		np1 = addr2ptr(np1);
	case PTR:
		return parithmetic(op, np1, np2);
	default:
	incorrect:
		error("incorrect arithmetic operands");
	}

	return bincode(op, np1->type, np1, np2);
}

/*
 * FIXME:
 * Pointers to the same basic type after removing type qualifiers
 * can be compared. It means that utype pointer must be a field
 * of the type, not of the Node, because in other case is
 * hard doing this check
 */
static Node *
pcompare(char op, Node *np1, Node *np2)
{
	if (np2->typeop == INT && np2->b.symbol && np2->u.sym->u.i == 0) {
		np2 = castcode(np2, pvoidtype);
	} else if (np2->typeop != PTR) {
		error("incompatibles type in comparision");
	} else {
		warn(options.pcompare,
		     "comparision between different pointer types");
	}

	return bincode(op, np1->type, np1, np2);
}

static Node *
compare(char op, Node *np1, Node *np2)
{
	np1 = eval(np1);
	np2 = eval(np2);
	switch (np1->typeop) {
	case INT: case FLOAT:
		switch (np1->typeop) {
		case INT: case FLOAT:
			typeconv(&np1, &np2);
			break;
		case ARY: case FTN:
			np2 = addr2ptr(np2);
		case PTR:
			return pcompare(op, np2, np1);
		default:
			goto nocompat;
		}
		break;
	case ARY: case FTN:
		np1 = addr2ptr(np1);
	case PTR:
		return pcompare(op, np1, np2);
	default:
	nocompat:
		error("incompatibles type in comparision");
	}

	return bincode(op, inttype, np1, np2);
}

static Node *
exp2cond(Node *np, char neg)
{
	if (ISNODECMP(np)) {
		np->u.op ^= neg;
		return np;
	}

	return compare(ONE ^ neg, np, symcode(zero));
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
	char *err;

	if (np1->typeop != INT && np2->typeop != INT)
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

Node *
iszero(Node *np)
{
	if (ISNODECMP(np))
		return np;
	return compare(ONE, np, symcode(zero));
}

static Node *
assignop(char op, Node *np1, Node *np2)
{
	if ((np2 = convert(np2, np1->type, 0)) == NULL)
		error("incompatible types when assigning");
	return bincode(op, np1->type, np1, np2);
}

static Node *
incdec(Node *np, char op)
{
	char *err;
	Type *tp = np->utype;

	if (!np->b.lvalue)
		goto nolvalue;
	if (np->utype == voidtype)
		goto voidassign;
	if (isconst(tp->op))
		goto const_mod;

	switch (np->typeop) {
	case PTR:
		if (!tp->defined)
			goto nocomplete;
	case INT: case FLOAT:
		return arithmetic(op, np, symcode(one));
	default:
		goto bad_type;
	}

voidassign:
	err = "invalid use of void expression";
	goto error;
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
address(char op, Node *np)
{
	char *err;

	if (!np->b.lvalue)
		goto no_lvalue;
	if (np->b.symbol && np->u.sym->s.isregister)
		goto reg_address;
	return unarycode(op, mktype(np->type, PTR, NULL, 0), np);

no_lvalue:
	err = "lvalue required in unary expression";
	goto error;
reg_address:
	err = "address of register variable '%s' requested";
error:
	error(err);
}

static Node *
content(char op, Node *np)
{
	switch (np->typeop) {
	case ARY: case FTN:
		np = addr2ptr(np);
	case PTR:
		np = unarycode(op, np->utype->type, np);
		np->b.lvalue = 1;
		return np;
	default:
		error("invalid argument of unary '*'");
	}
}

static Node *
negation(char op, Node *np)
{
	switch (np->typeop) {
	case FTN: case ARY:
		np = addr2ptr(np);
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
		np = symcode(yylval.sym);
		if (yytoken == IDEN) {
			np->b.lvalue = 1;
			np->b.constant = 0;
		}
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
			np1 = incdec(np1,  (yytoken == INC) ? OINC : ODEC);
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
	register Node *(*fun)(char, Node *);
	register char op;
	Type *tp;
	char *err;

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
		op = (yytoken == INC) ? OA_ADD : OA_SUB;
		next();
		return incdec(unary(), op);
	case '!': op = 0; fun = negation; break;
	case '+': op = OADD; fun = integeruop; break;
	case '-': op = OSUB; fun = integeruop; break;
	case '~': op = ONEG; fun = integeruop; break;
	case '&': op = OADDR; fun = address; break;
	case '*': op = OPTR; fun = content; break;
	default: return postfix();
	}

	next();
	return (*fun)(op, cast());
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
			np1 = eval(cast());
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
	register Node *np, *(*fun)(char, Node *, Node *);
	register char op;

	np = cast();
	for (;;) {
		switch (yytoken) {
		case '*': op = OMUL; fun = arithmetic; break;
		case '/': op = ODIV; fun = arithmetic; break;
		case '%': op = OMOD; fun = integerop;   break;
		default: return np;
		}
		next();
		np = (*fun)(op, np, cast());
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
		np = integerop(op, np, add());
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
		np = integerop(OBAND, np, eq());
	return np;
}

static Node *
bit_xor(void)
{
	register Node *np;

	np = bit_and();
	while (accept('^'))
		np = integerop(OBXOR,  np, bit_and());
	return np;
}

static Node *
bit_or(void)
{
	register Node *np;

	np = bit_xor();
	while (accept('|'))
		np = integerop(OBOR, np, bit_xor());
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

	np = or();
	while (accept('?')) {
		ifyes = promote(expr());
		expect(':');
		ifno = promote(ternary());
		typeconv(&ifyes, &ifno);
		np = ternarycode(iszero(np), ifyes, ifno);
	}
	return np;
}

static Node *
assign(void)
{
	register Node *np, *(*fun)(char , Node *, Node *);
	register char op;
	char *err;

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
		if (!np->b.lvalue)
			goto nolvalue;
		if (np->utype == voidtype)
			goto voidassign;
		if (isconst(np->type->op))
			goto const_mod;
		next();
		np = (fun)(op, np, eval(assign()));
	}
voidassign:
	err = "invalid use of void expression";
	goto error;
const_mod:
	err = "const value modified";
	goto error;
nolvalue:
	err = "lvalue required as left operand of assignment";
	goto error;
error:
	error(err);
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
