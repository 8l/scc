#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"
#include "cc1.h"


Node *expr(void);

static bool
isnodecmp(int op)
{
	switch (op) {
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

#define SYMICMP(sym, val) (((sym)->type->sign) ?         \
	(sym)->u.i == (val) : (sym)->u.u == (val))

#define FOLDINT(sym, ls, rs, op) (((sym)->type->sign) ? \
	((sym)->u.i = ((ls)->u.i op (rs)->u.i)) :       \
	((sym)->u.u = ((ls)->u.u op (rs)->u.u)))

#define CMPISYM(sym, ls, rs, op) (((sym)->type->sign) ? \
	((ls)->u.i op (rs)->u.i) : ((ls)->u.u op (rs)->u.u))

static Node *
simplify(unsigned char op, Type *tp, Node *lp, Node *rp)
{
	Symbol *sym, *ls, *rs, aux;
	int iszero, isone, noconst = 0;

	if (!lp->constant && !rp->constant)
		goto no_simplify;
	if (!rp->constant) {
		Node *np;
		np = lp;
		lp = rp;
		rp = np;
	}
	if (!lp->constant)
		noconst = 1;

	ls = lp->sym, rs = rp->sym;
	aux.type = tp;

	/* TODO: Add overflow checkings */

	if (isnodecmp(op)) {
		/*
		 * Comparision nodes have integer type
		 * but the operands can have different
		 * type.
		 */
		switch (BTYPE(lp)) {
		case INT:   goto cmp_integers;
		case FLOAT: goto cmp_floats;
		default:    goto no_simplify;
		}
	}

	switch (tp->op) {
	case PTR:
	case INT:
	cmp_integers:
		iszero = SYMICMP(rs, 0);
		isone = SYMICMP(rs, 1);
		switch (op) {
		case OADD:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, +);
			break;
		case OSUB:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, -);
			break;
		case OMUL:
			if (isone)
				return lp;
			if (iszero)
				return constnode(zero);
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, *);
			break;
		case ODIV:
			if (isone)
				return lp;
			if (iszero)
				goto division_by_0;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, /);
			break;
		case OMOD:
			if (iszero)
				goto division_by_0;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, %);
			break;
		case OSHL:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, <<);
			break;
		case OSHR:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, >>);
			break;
		case OBAND:
			if (SYMICMP(rs, ~0))
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, &);
			break;
		case OBXOR:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, ^);
			break;
		case OBOR:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, |);
			break;
		case OAND:
			if (!iszero)
				return lp;
			/* TODO: What happens with something like f(0) && 0? */
			if (noconst)
				goto no_simplify;
			FOLDINT(&aux, ls, rs, &&);
			break;
		case OOR:
			if (iszero)
				return lp;
			if (noconst)
				goto no_simplify;
			/* TODO: What happens with something like f(0) || 1? */
			FOLDINT(&aux, ls, rs, ||);
			break;
		case OLT:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, <);
			break;
		case OGT:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, >);
			break;
		case OGE:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, >=);
			break;
		case OLE:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, <=);
			break;
		case OEQ:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, ==);
			break;
		case ONE:
			/* TODO: what happens with signess? */
			if (noconst)
				goto no_simplify;
			aux.u.i = CMPISYM(&aux, ls, rs, !=);
			break;
		}
		break;
	case FLOAT:
	cmp_floats:
		/* TODO: Add algebraic reductions for floats */
		switch (op) {
		case OADD:
			aux.u.f = ls->u.f + rs->u.f;
			break;
		case OSUB:
			aux.u.f = ls->u.f - rs->u.f;
			break;
		case OMUL:
			aux.u.f = ls->u.f * rs->u.f;
			break;
		case ODIV:
			if (rs->u.f == 0.0)
				goto division_by_0;
			aux.u.f = ls->u.f / rs->u.f;
			break;
		case OLT:
			aux.u.i = ls->u.f < rs->u.f;
			break;
		case OGT:
			aux.u.i = ls->u.f > rs->u.f;
			break;
		case OGE:
			aux.u.i = ls->u.f >= rs->u.f;
			break;
		case OLE:
			aux.u.i = ls->u.f <= rs->u.f;
			break;
		case OEQ:
			aux.u.i = ls->u.f == rs->u.f;
			break;
		case ONE:
			aux.u.i = ls->u.f != rs->u.f;
			break;
		}
		break;
	default:
		goto no_simplify;
	}

	sym = newsym(NS_IDEN);
	sym->type = tp;
	sym->u = aux.u;
	return constnode(sym);

division_by_0:
	warn("division by 0");

no_simplify:
	return node(op, tp, lp, rp);
}

#define UFOLDINT(sym, ls, op) (((sym)->type->sign) ?     \
	((sym)->u.i = (op (ls)->u.i)) :                  \
	((sym)->u.u = (op (ls)->u.u)))

static Node *
usimplify(unsigned char op, Type *tp, Node *np)
{
	Symbol *sym, *ns, aux;

	if (!np->constant)
		goto no_simplify;
	ns = np->sym;
	aux.type = tp;

	switch (tp->op) {
	case INT:
		switch (op) {
		case ONEG:
			UFOLDINT(&aux, ns, -);
			break;
		case OCPL:
			UFOLDINT(&aux, ns, ~);
			break;
		default:
			goto no_simplify;
		}
		break;
	case FLOAT:
		if (op != ONEG)
			goto no_simplify;
		aux.u.f = -ns->u.f;
		break;
	default:
		goto no_simplify;
	}

	sym = newsym(NS_IDEN);
	sym->type = tp;
	sym->u = aux.u;
	return constnode(sym);

no_simplify:
	return node(op, tp, np, NULL);
}

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

static Node *
decay(Node *np)
{
	Type *tp = np->type;

	switch (tp->op) {
	case ARY:
		tp = tp->type;
		if (np->op == OPTR) {
			Node *new = np->left;
			free(np);
			new->type = mktype(tp, PTR, 0, NULL);
			return new;
		}
	case FTN:
		break;
	default:
		return np;
	}

	return node(OADDR, mktype(tp, PTR, 0, NULL), np, NULL);
}

Node *
eval(Node *np)
{
	Node *p;

	if (!np)
		return NULL;

	np = decay(np);
	if (!isnodecmp(np->op))
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
	return simplify(op, lp->type, lp, rp);
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
		return usimplify(op, np->type, np);
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
	return usimplify(op, np->type, np);
}

static Node *
constconv(Node *np, Type *newtp)
{
	Type *oldtp = np->type;
	Symbol aux, *sym, *osym = np->sym;

	switch (newtp->op) {
	case PTR:
	case INT:
	case ENUM:
		switch (oldtp->op) {
		case PTR:
		case INT:
		case ENUM:
			if (newtp->sign == oldtp->sign)
				aux.u = osym->u;
			if (newtp->sign && !oldtp->sign)
				aux.u.i = osym->u.u;
			else if (!newtp->sign && oldtp->sign)
				aux.u.u = osym->u.u;
			break;
		case FLOAT:
			if (newtp->sign)
				aux.u.i = osym->u.f;
			else
				aux.u.u = osym->u.f;
			break;
		default:
			return NULL;
		}
		break;
	case FLOAT:
		aux.u.f = (oldtp->sign) ? osym->u.i : osym->u.u;
		break;
	default:
		return NULL;
	}

	sym = newsym(NS_IDEN);
	np->type = sym->type = newtp;
	np->sym = sym;
	sym->u = aux.u;

	return np;
}

/*
 * Convert a Node to a type
 */
Node *
convert(Node *np, Type *newtp, char iscast)
{
	Type *oldtp = np->type;
	Node *p;

	if (eqtype(newtp, oldtp))
		return np;
	if (np->constant && (p = constconv(np, newtp)) != NULL)
		return p;

	switch (oldtp->op) {
	case ENUM:
	case INT:
	case FLOAT:
		switch (newtp->op) {
		case PTR:
			if (!iscast || oldtp->op == FLOAT)
				return NULL;
			/* PASSTHROUGH */
		case INT:
		case FLOAT:
		case ENUM:
		case VOID:
			break;
		default:
			return NULL;
		}
		break;
	case PTR:
		switch (newtp->op) {
		case ENUM:
		case INT:
		case VOID:
			if (!iscast)
				return NULL;
			break;
		case PTR:
			if (iscast ||
			    newtp == pvoidtype ||
			    oldtp == pvoidtype) {
				/* TODO:
				 * we assume conversion between pointers
				 * do not need any operation, but due to
				 * alignment problems that may be false
				 */
				np->type = newtp;
				return np;
			}
		default:
			return NULL;
		}
	default:
			return NULL;
	}
	return node(OCAST, newtp, np, NULL);
}

static Node *
parithmetic(char op, Node *lp, Node *rp)
{
	Type *tp;
	Node *size;

	tp = lp->type;
	size = sizeofnode(tp->type);

	if (op == OSUB && BTYPE(rp) == PTR) {
		if (tp != rp->type)
			goto incorrect;
		lp = node(OSUB, inttype, lp, rp);
		return node(ODIV, inttype, lp, size);
	}
	if (BTYPE(rp) != INT)
		goto incorrect;

	rp = convert(promote(rp), sizettype, 0);
	rp = simplify(OMUL, sizettype, rp, size);
	rp = convert(rp, tp, 1);

	return simplify(OADD, tp, lp, rp);

incorrect:
	errorp("incorrect arithmetic operands");
	return node(OADD, tp, lp, rp);
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
		case PTR:
			if (op == OADD || op == OSUB)
				return parithmetic(op, rp, lp);
		default:
			goto incorrect;
		}
		break;
	case PTR:
		return parithmetic(op, lp, rp);
	default:
	incorrect:
		errorp("incorrect arithmetic operands");
	}

	return simplify(op, lp->type, lp, rp);
}

static Node *
pcompare(char op, Node *lp, Node *rp)
{
	switch (BTYPE(rp)) {
	case INT:
		if (rp->constant && SYMICMP(rp->sym, 0))
			rp = node(OCAST, pvoidtype, rp, NULL);
		break;
	case PTR:
		if (lp->type != rp->type)
			warn("comparision between different pointer types");
		break;
	default:
		errorp("incompatibles type in comparision");
	}

	return node(op, inttype, lp, rp);
}

static Node *
compare(char op, Node *lp, Node *rp)
{
	lp = eval(lp);
	rp = eval(rp);
	switch (BTYPE(lp)) {
	case INT:
	case FLOAT:
		switch (BTYPE(lp)) {
		case INT:
		case FLOAT:
			typeconv(&lp, &rp);
			break;
		case PTR:
			return pcompare(op, rp, lp);
		default:
			goto nocompat;
		}
		break;
	case PTR:
		return pcompare(op, lp, rp);
	default:
	nocompat:
		errorp("incompatibles type in comparision");
	}

	return simplify(op, inttype, lp, rp);
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
	default:   return np;
	}
	np->op = op;
	return np;
}

Node *
exp2cond(Node *np, char neg)
{
	np = decay(np);
	if (isnodecmp(np->op))
		return (neg) ? negate(np) : np;
	return compare((neg) ?  OEQ : ONE, np, constnode(zero));
}

static Node *
logic(char op, Node *lp, Node *rp)
{
	lp = exp2cond(lp, 0);
	rp = exp2cond(rp, 0);
	return simplify(op, inttype, lp, rp);
}

static Node *
field(Node *np)
{
	Symbol *sym;

	switch (BTYPE(np)) {
	case STRUCT:
	case UNION:
		namespace = np->type->ns;
		next();
		namespace = NS_IDEN;

		if (yytoken != IDEN)
			unexpected();
		if ((sym = yylval.sym) == NULL)
			error("incorrect field in struct/union");
		next();
		np = node(OFIELD, sym->type, np, varnode(sym));
		np->lvalue = 1;
		return np;
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
		errorp("subscripted value is neither array nor pointer");
	lp =  node(OPTR, tp->type, lp, NULL);
	lp->lvalue = 1;
	return lp;
}

static Node *
assignop(char op, Node *lp, Node *rp)
{
	lp = eval(lp);
	rp = eval(rp);

	if (BTYPE(rp) == INT && BTYPE(lp) == PTR &&
	    rp->constant && SYMICMP(rp->sym, 0)) {
		rp = node(OCAST, pvoidtype, rp, NULL);
	} else if ((rp = convert(rp, lp->type, 0)) == NULL) {
		errorp("incompatible types when assigning");
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
	if (np->op == OPTR) {
		Node *new = np->left;
		free(np);
		return new;
	}
	return node(op, mktype(np->type, PTR, 0, NULL), np, NULL);
}

static Node *
content(char op, Node *np)
{
	switch (BTYPE(np)) {
	case ARY:
	case FTN:
	case PTR:
		if (np->op == OADDR) {
			Node *new = np->left;
			free(np);
			return new;
		}
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
		if ((yylval.sym->flags & ISDECLARED) == 0) {
			yylval.sym->type = inttype;
			yylval.sym->flags |= ISDECLARED;
			error("'%s' undeclared", yytext);
		}
		yylval.sym->flags |= ISUSED;
		np = varnode(yylval.sym);
		next();
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
	int n;
	Node *par = NULL, *arg;
	Type **targs, *tp = np->type;

	if (tp->op == PTR && tp->type->op == FTN) {
		np = content(OPTR, np);
		tp = np->type;
	}
	if (tp->op != FTN)
		error("function or function pointer expected");
	targs = tp->p.pars;

	expect('(');

	if ((n = tp->n.elem) > 0) {
		do {
			if ((arg = eval(assign())) == NULL)
				unexpected();
			if ((arg = convert(arg, *targs++, 0)) == NULL)
				goto bad_type;
			par = node(OPAR, arg->type, par, arg);
		} while (--n && accept(','));
	}

	if (n > 0)
		error("too few arguments in function call");
	if (yytoken == ',')
		error("too many arguments in function call");

	expect(')');
	return node(OCALL, np->type->type, np, par);

bad_type:
	error("incompatible type for argument %d in function call",
	      tp->n.elem - n + 1);
}

static Node *
postfix(Node *lp)
{
	Node *rp;

	if (!lp)
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
	default:  return postfix(NULL);
	}

	next();
	return (*fun)(op, cast());
}

static Node *
cast(void)
{
	Node *lp, *rp;
	Type *tp;
	static int nested;

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
		if (nested == NR_SUBEXPR)
			error("too expressions nested by parentheses");
		rp = expr();
		expect(')');
		rp = postfix(rp);
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
		if (cond->constant) {
			TINT i = cond->sym->u.i;

			freetree(cond);
			if (i == 0) {
				freetree(ifyes);
				return ifno;
			} else {
				freetree(ifno);
				return ifyes;
			}
		}
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
		np = (fun)(op, np, assign());
	}
}

Node *
constexpr(void)
{
	Node *np;

	np = ternary();
	if (!np->constant) {
		freetree(np);
		return NULL;
	}
	return np;
}

Node *
iconstexpr(void)
{
	Node *np;

	if ((np = constexpr()) == NULL)
		return NULL;

	if (np->type->op != INT) {
		freetree(np);
		return NULL;
	}

	return convert(np, inttype, 0);
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

Node *
condition(void)
{
	Node *np;

	expect('(');
	np = exp2cond(expr(), 0);
	if (np->constant)
		warn("conditional expression is constant");
	expect(')');

	return np;
}
