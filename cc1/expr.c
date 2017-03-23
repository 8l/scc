/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc1/expr.c";
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstd.h>
#include "../inc/cc.h"
#include "cc1.h"

#define XCHG(lp, rp, np) (np = lp, lp = rp, rp = np)

static Node *xexpr(void), *xassign(void);

int
cmpnode(Node *np, TUINT val)
{
	Symbol *sym;
	Type *tp;
	TUINT mask, nodeval;

	if (!np || !(np->flags & NCONST) || !np->sym)
		return 0;
	sym = np->sym;
	tp = sym->type;

	switch (tp->op) {
	case PTR:
	case INT:
		mask = (val > 1) ? ones(np->type->size) : -1;
		nodeval = (tp->prop & TSIGNED) ? sym->u.i : sym->u.u;
		return (nodeval & mask) == (val & mask);
	case FLOAT:
		return sym->u.f == val;
	}
	return 0;
}

static Node *
promote(Node *np)
{
	Type *tp;
	Node *new;
	struct limits *lim, *ilim;

	tp = np->type;

	switch (tp->op) {
	case ENUM:
	case INT:
		if (tp->n.rank >= inttype->n.rank)
			return np;
		lim = getlimits(tp);
		ilim = getlimits(inttype);
		tp = (lim->max.i <= ilim->max.i) ? inttype : uinttype;
		break;
	case FLOAT:
		/* TODO: Add support for C99 float math */
		tp = doubletype;
		break;
	default:
		abort();
	}
	if ((new = convert(np, tp, 1)) != NULL)
		return new;
	return np;
}

static void
arithconv(Node **p1, Node **p2)
{
	int to = 0, s1, s2;
	unsigned r1, r2;
	Type *tp1, *tp2;
	Node *np1, *np2;
	struct limits *lp1, *lp2;

	np1 = promote(*p1);
	np2 = promote(*p2);

	tp1 = np1->type;
	tp2 = np2->type;

	if (tp1 == tp2)
		goto set_p1_p2;

	s1 = (tp1->prop & TSIGNED) != 0;
	r1 = tp1->n.rank;
	lp1 = getlimits(tp1);

	s2 = (tp2->prop & TSIGNED) != 0;
	r2 = tp2->n.rank;
	lp2 = getlimits(tp2);

	if (s1 == s2 || tp1->op == FLOAT || tp2->op == FLOAT) {
		to = r1 - r2;
	} else if (!s1) {
		if (r1 >= r2 || lp1->max.i >= lp2->max.i)
			to = 1;
		else
			to = -1;
	} else {
		if (r2 >= r1 || lp2->max.i >= lp1->max.i)
			to = -1;
		else
			to = 1;
	}

	if (to > 0)
		np2 = convert(np2, tp1, 1);
	else if (to < 0)
		np1 = convert(np1, tp2, 1);
		
set_p1_p2:
	*p1 = np1;
	*p2 = np2;
}

static int
null(Node *np)
{
	if (!(np->flags&NCONST) || np->type != pvoidtype)
		return 0;
	return cmpnode(np, 0);
}

static Node *
chkternary(Node *yes, Node *no)
{
	/*
	 * FIXME:
	 * We are ignoring type qualifiers here,
	 * but the standard has strong rules about this.
	 * take a look to 6.5.15
	 */

	if (!eqtype(yes->type, no->type, 1)) {
		if ((yes->type->prop & TARITH) && (no->type->prop & TARITH)) {
			arithconv(&yes, &no);
		} else if (yes->type->op != PTR && no->type->op != PTR) {
			goto wrong_type;
		} else {
			/* convert integer 0 to NULL */
			if ((yes->type->prop & TINTEGER) && cmpnode(yes, 0))
				yes = convert(yes, pvoidtype, 0);
			if ((no->type->prop & TINTEGER) && cmpnode(no, 0))
				no = convert(no, pvoidtype, 0);
			/*
			 * At this point the type of both should be
			 * a pointer to something, or we have don't
			 * compatible types
			 */
			if (yes->type->op != PTR || no->type->op != PTR)
				goto wrong_type;
			/*
			 * If we have a null pointer constant then
			 * convert to the another type
			 */
			if (null(yes))
				yes = convert(yes, no->type, 0);
			if (null(no))
				no = convert(no, yes->type, 0);

			if (!eqtype(yes->type, no->type, 1))
				goto wrong_type;
		}
	}
	return node(OCOLON, yes->type, yes, no);

wrong_type:
	errorp("type mismatch in conditional expression");
	freetree(yes);
	freetree(no);
	return constnode(zero);
}

static void
chklvalue(Node *np)
{
	if (!(np->flags & NLVAL))
		errorp("lvalue required in operation");
	if (np->type == voidtype)
		errorp("invalid use of void expression");
}

Node *
decay(Node *np)
{
	Node *new;
	Type *tp = np->type;

	switch (tp->op) {
	case ARY:
		tp = tp->type;
		if (np->op == OPTR) {
			new = np->left;
			free(np);
			new->type = mktype(tp, PTR, 0, NULL);
			return new;
		}
	case FTN:
		new = node(OADDR, mktype(tp, PTR, 0, NULL), np, NULL);
		if (np->sym && np->sym->flags & (SGLOBAL|SLOCAL|SPRIVATE))
			new->flags |= NCONST;
		return new;
	default:
		return np;
	}
}

static Node *
integerop(int op, Node *lp, Node *rp)
{
	if (!(lp->type->prop & TINTEGER) || !(rp->type->prop & TINTEGER))
		error("operator requires integer operands");
	arithconv(&lp, &rp);
	return node(op, lp->type, lp, rp);
}

static Node *
integeruop(int op, Node *np)
{
	if (!(np->type->prop & TINTEGER))
		error("unary operator requires integer operand");
	np = promote(np);
	return node(op, np->type, np, NULL);
}

static Node *
numericaluop(int op, Node *np)
{
	if (!(np->type->prop & TARITH))
		error("unary operator requires numerical operand");
	np = promote(np);
	return node(op, np->type, np, NULL);
}

Node *
convert(Node *np, Type *newtp, int iscast)
{
	Type *oldtp = np->type;

	if (eqtype(newtp, oldtp, 0))
		return np;

	switch (oldtp->op) {
	case ENUM:
	case INT:
	case FLOAT:
		switch (newtp->op) {
		case PTR:
			if (oldtp->op == FLOAT || !cmpnode(np, 0) && !iscast)
				return NULL;
			/* PASSTHROUGH */
		case INT:
		case FLOAT:
		case ENUM:
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
			if (eqtype(newtp, oldtp, 1) ||
			    iscast ||
			    newtp == pvoidtype || oldtp == pvoidtype) {
				np->type = newtp;
				return np;
			}
		default:
			return NULL;
		}
		break;
	default:
		return NULL;
	}
	return node(OCAST, newtp, np, NULL);
}

static Node *
parithmetic(int op, Node *lp, Node *rp)
{
	Type *tp;
	Node *size, *np;

	if (lp->type->op != PTR)
		XCHG(lp, rp, np);

	tp = rp->type;
	if (tp->op == PTR && !(tp->type->prop & TDEFINED))
		goto incomplete;
	tp = lp->type;
	if (!(tp->type->prop & TDEFINED))
		goto incomplete;
	size = sizeofnode(tp->type);

	if (op == OSUB && BTYPE(rp) == PTR) {
		if ((rp = convert(rp, lp->type, 0)) == NULL)
			goto incorrect;
		lp = node(OSUB, pdifftype, lp, rp);
		return node(ODIV, inttype, lp, size);
	}
	if (!(rp->type->prop & TINTEGER))
		goto incorrect;

	rp = convert(promote(rp), sizettype, 0);
	rp = node(OMUL, sizettype, rp, size);
	rp = convert(rp, tp, 1);

	return node(op, tp, lp, rp);

incomplete:
	errorp("invalid use of undefined type");
	return lp;
incorrect:
	errorp("incorrect arithmetic operands");
	return lp;

}

static Node *
arithmetic(int op, Node *lp, Node *rp)
{
	Type *ltp = lp->type, *rtp = rp->type;

	if ((ltp->prop & TARITH) && (rtp->prop & TARITH)) {
		arithconv(&lp, &rp);
		return node(op, lp->type, lp, rp);
	} else if ((ltp->op == PTR || rtp->op == PTR)) {
		switch (op) {
		case OADD:
		case OSUB:
		case OA_ADD:
		case OA_SUB:
		case OINC:
		case ODEC:
			return parithmetic(op, lp, rp);
		}
	}
	errorp("incorrect arithmetic operands");
}

static Node *
pcompare(int op, Node *lp, Node *rp)
{
	Node *np;

	if (lp->type->prop & TINTEGER)
		XCHG(lp, rp, np);
	else if (eqtype(lp->type, pvoidtype, 1))
		XCHG(lp, rp, np);

	if ((np = convert(rp, lp->type, 0)) != NULL)
		rp = np;
	else
		errorp("incompatible types in comparison");
	return convert(node(op, pvoidtype, lp, rp), inttype, 1);
}

static Node *
compare(int op, Node *lp, Node *rp)
{
	Type *ltp, *rtp;

	ltp = lp->type;
	rtp = rp->type;

	if (ltp->op == PTR || rtp->op == PTR) {
		return pcompare(op, rp, lp);
	} else if ((ltp->prop & TARITH) && (rtp->prop & TARITH)) {
		arithconv(&lp, &rp);
		return convert(node(op, lp->type, lp, rp), inttype, 1);;
	} else {
		errorp("incompatible types in comparison");
		freetree(lp);
		freetree(rp);
		return constnode(zero);
	}
}

int
negop(int op)
{
	switch (op) {
	case OEQ:  return ONE;
	case ONE:  return OEQ;
	case OLT:  return OGE;
	case OGE:  return OLT;
	case OLE:  return OGT;
	case OGT:  return OLE;
	default:   abort();
	}
	return op;
}

static Node *
exp2cond(Node *np, int neg)
{
	if (np->type->prop & TAGGREG) {
		errorp("used struct/union type value where scalar is required");
		return constnode(zero);
	}
	switch (np->op) {
	case ONEG:
	case OOR:
	case OAND:
		return (neg) ? node(ONEG, inttype, np, NULL) : np;
	case OEQ:
	case ONE:
	case OLT:
	case OGE:
	case OLE:
	case OGT:
		if (neg)
			np->op = negop(np->op);
		return np;
	default:
		return compare((neg) ?  OEQ : ONE, np, constnode(zero));
	}
}

static Node *
logic(int op, Node *lp, Node *rp)
{
	lp = exp2cond(lp, 0);
	rp = exp2cond(rp, 0);
	return node(op, inttype, lp, rp);
}

static Node *
field(Node *np)
{
	Symbol *sym;

	namespace = np->type->ns;
	next();
	namespace = NS_IDEN;

	sym = yylval.sym;
	if (yytoken != IDEN)
		unexpected();
	next();

	if (!(np->type->prop & TAGGREG)) {
		errorp("request for member '%s' in something not a structure or union",
		      yylval.sym->name);
		goto free_np;
	}
	if ((sym->flags & SDECLARED) == 0) {
		errorp("incorrect field in struct/union");
		goto free_np;
	}
	np = node(OFIELD, sym->type, np, varnode(sym));
	np->flags |= NLVAL;
	return np;

free_np:
	freetree(np);
	return constnode(zero);
}

static Node *
content(int op, Node *np)
{
	if (BTYPE(np) != PTR) {
		errorp("invalid argument of memory indirection");
	} else {
		if (np->op == OADDR) {
			Node *new = np->left;
			new->type = np->type->type;
			free(np);
			np = new;
		} else {
			np = node(op, np->type->type, np, NULL);
		}
		np->flags |= NLVAL;
	}
	return np;
}

static Node *
array(Node *lp, Node *rp)
{
	Type *tp;
	Node *np;

	if (!(lp->type->prop & TINTEGER) && !(rp->type->prop & TINTEGER))
		error("array subscript is not an integer");
	np = arithmetic(OADD, lp, rp);
	tp = np->type;
	if (tp->op != PTR)
		errorp("subscripted value is neither array nor pointer");
	return content(OPTR, np);
}

static Node *
assignop(int op, Node *lp, Node *rp)
{
	if ((rp = convert(rp, lp->type, 0)) == NULL) {
		errorp("incompatible types when assigning");
		return lp;
	}

	return node(op, lp->type, lp, rp);
}

static Node *
incdec(Node *np, int op)
{
	Type *tp = np->type;
	Node *inc;

	chklvalue(np);
	np->flags |= NEFFECT;

	if (!(tp->prop & TDEFINED)) {
		errorp("invalid use of undefined type");
		return np;
	} else if (tp->op == PTR && !(tp->type->prop & TDEFINED)) {
		errorp("%s of pointer to an incomplete type",
		       (op == OINC || op == OA_ADD) ? "increment" : "decrement");
		return np;
	} else if (tp->op == PTR || (tp->prop & TARITH)) {
		inc = constnode(one);
	} else {
		errorp("wrong type argument to increment or decrement");
		return np;
	}
	return arithmetic(op, np, inc);
}

static Node *
address(int op, Node *np)
{
	Node *new;

	/*
	 * ansi c accepts & applied to a function name, and it generates
	 * a function pointer
	 */
	if (np->op == OSYM) {
		if (np->type->op == FTN)
			return decay(np);
		if (np->type->op == ARY)
			goto dont_check_lvalue;
	}
	chklvalue(np);

dont_check_lvalue:
	if (np->sym && (np->sym->flags & SREGISTER))
		errorp("address of register variable '%s' requested", yytext);
	new = node(op, mktype(np->type, PTR, 0, NULL), np, NULL);
	if (np->sym && np->sym->flags & (SGLOBAL|SLOCAL|SPRIVATE))
		new->flags |= NCONST;
	return new;
}

static Node *
negation(int op, Node *np)
{
	if (!(np->type->prop & TARITH) && np->type->op != PTR) {
		errorp("invalid argument of unary '!'");
		return constnode(zero);
	}
	return exp2cond(np, 1);
}

static Symbol *
notdefined(Symbol *sym)
{
	int isdef;

	if (namespace == NS_CPP && !strcmp(sym->name, "defined")) {
		disexpand = 1;
		next();
		expect('(');
		sym = yylval.sym;
		expect(IDEN);
		expect(')');

		isdef = (sym->flags & SDECLARED) != 0;
		sym = newsym(NS_IDEN, NULL);
		sym->type = inttype;
		sym->flags |= SCONSTANT;
		sym->u.i = isdef;
		disexpand = 0;
		return sym;
	}
	errorp("'%s' undeclared", yytext);
	sym->type = inttype;
	return install(sym->ns, yylval.sym);
}

static Symbol *
adjstrings(Symbol *sym)
{
	char *s, *t;
	size_t len, n;
	Type *tp;

	tp = sym->type;
	s = sym->u.s;
	for (len = strlen(s);; len += n) {
		next();
		if (yytoken != STRING)
			break;
		t = yylval.sym->u.s;
		n = strlen(t);
		s = xrealloc(s, len + n + 1);
		memcpy(s+len, t, n);
		s[len + n] = '\0';
		killsym(yylval.sym);
	}
	++len;
	if (tp->n.elem != len) {
		sym->type = mktype(chartype, ARY, len, NULL);
		sym->u.s = s;
	}
	return sym;
}

/*************************************************************
 * grammar functions                                         *
 *************************************************************/
static Node *
primary(void)
{
	Node *np;
	Symbol *sym;
	Node *(*fun)(Symbol *);

	sym = yylval.sym;
	switch (yytoken) {
	case STRING:
		np = constnode(adjstrings(sym));
		sym->flags |= SHASINIT;
		emit(ODECL, sym);
		emit(OINIT, np);
		return varnode(sym);
	case BUILTIN:
		fun = sym->u.fun;
		next();
		expect('(');
		np = (*fun)(sym);
		expect(')');

		/* do not call to next */
		return np;
	case CONSTANT:
		np = constnode(sym);
		break;
	case IDEN:
		assert((sym->flags & SCONSTANT) == 0);
		if ((sym->flags & SDECLARED) == 0) {
			if (namespace == NS_CPP) {
				np = constnode(zero);
				break;
			}
			sym = notdefined(sym);
		}
		sym->flags |= SUSED;
		np = varnode(sym);
		break;
	default:
		unexpected();
	}
	next();

	return np;
}

static Node *
arguments(Node *np)
{
	int toomany, n, op;
	Node *par = NULL, *arg;
	Type *argtype, **targs, *tp = np->type, *rettype;

	if (tp->op == PTR && tp->type->op == FTN) {
		np = content(OPTR, np);
		tp = np->type;
	}
	if (tp->op != FTN) {
		targs = (Type *[]) {ellipsistype};
		n = 1;
		rettype = inttype;
		errorp("function or function pointer expected");
	} else {
		targs = tp->p.pars;
		n = tp->n.elem;
		rettype = tp->type;
	}

	expect('(');
	if (yytoken == ')')
		goto no_pars;
	toomany = 0;

	do {
		arg = xassign();
		argtype = *targs;
		if (argtype == ellipsistype) {
			n = 0;
			switch (arg->type->op) {
			case INT:
				arg = promote(arg);
				break;
			case FLOAT:
				if (arg->type == floattype)
					arg = convert(arg, doubletype, 1);
				break;
			}
			par = node(OPAR, arg->type, par, arg);
			continue;
		}
		if (--n < 0) {
			if (!toomany)
				errorp("too many arguments in function call");
			toomany = 1;
			continue;
		}
		++targs;
		if ((arg = convert(arg, argtype, 0)) != NULL) {
			par = node(OPAR, arg->type, par, arg);
			continue;
		}
		errorp("incompatible type for argument %d in function call",
		       tp->n.elem - n + 1);
	} while (accept(','));

no_pars:
	expect(')');
	if (n > 0 && *targs != ellipsistype)
		errorp("too few arguments in function call");

	op = (tp->prop&TELLIPSIS) ? OCALLE : OCALL;
	return node(op, rettype, np, par);
}

static Node *unary(int);

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
		tp = typeof(unary(0));
		break;
	}
	expect(')');
	return tp;
}

static Node *
postfix(Node *lp)
{
	Node *rp;

	for (;;) {
		switch (yytoken) {
		case '[':
		case DEC:
		case INC:
		case INDIR:
		case '.':
		case '(':
			lp = decay(lp);
			switch (yytoken) {
			case '[':
				next();
				rp = xexpr();
				expect(']');
				lp = array(lp, rp);
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
				lp->flags |= NEFFECT;
				break;
			}
			break;
		default:
			return lp;
		}
	}
}

static Node *cast(int);

static Node *
unary(int needdecay)
{
	Node *(*fun)(int, Node *), *np;
	Symbol *sym;
	int op;
	Type *tp;
	int paren;

	switch (yytoken) {
	case '!': op = 0;     fun = negation;     break;
	case '+': op = OADD;  fun = numericaluop; break;
	case '-': op = OSNEG; fun = numericaluop; break;
	case '~': op = OCPL;  fun = integeruop;   break;
	case '&': op = OADDR; fun = address;      break;
	case '*': op = OPTR;  fun = content;      break;
	case SIZEOF:
		next();
		tp = (yytoken == '(') ? sizeexp() : typeof(unary(0));
		if (!(tp->prop & TDEFINED))
			errorp("sizeof applied to an incomplete type");
		return sizeofnode(tp);
	case INC:
	case DEC:
		op = (yytoken == INC) ? OA_ADD : OA_SUB;
		next();
		np = incdec(unary(1), op);
		goto chk_decay;
	case IDEN:
	case TYPEIDEN:
		if (lexmode != CPPMODE || strcmp(yylval.sym->name, "defined"))
			goto call_postfix;
		disexpand = 1;
		next();
		paren = accept('(');
		if (yytoken != IDEN && yytoken != TYPEIDEN)
			cpperror("operator 'defined' requires an identifier");
		if (yytoken == TYPEIDEN || !(yylval.sym->flags & SDECLARED))
			sym = zero;
		else
			sym = one;
		disexpand = 0;
		next();
		if (paren)
			expect(')');
		return constnode(sym);
	default:
	call_postfix:
		np = postfix(primary());
		goto chk_decay;
	}

	next();
	np = (*fun)(op, cast(op != OADDR));

chk_decay:
	if (needdecay)
		np = decay(np);
	return np;
}

static Node *
cast(int needdecay)
{
	Node *lp, *rp;
	Type *tp;
	static int nested;

	if (!accept('('))
		return unary(needdecay);

	switch (yytoken) {
	case TQUALIFIER:
	case TYPE:
	case TYPEIDEN:
		tp = typename();
		expect(')');

		if (yytoken == '{')
			return initlist(tp);

		switch (tp->op) {
		case ARY:
			error("cast specifies an array type");
		default:
			lp = cast(needdecay);
			if ((rp = convert(lp,  tp, 1)) == NULL)
				error("bad type conversion requested");
			rp->flags &= ~NLVAL;
			rp->flags |= lp->flags & NLVAL;
		}
		break;
	default:
		if (nested == NR_SUBEXPR)
			error("too many expressions nested by parentheses");
		++nested;
		rp = xexpr();
		--nested;
		expect(')');
		rp = postfix(rp);
		break;
	}

	return rp;
}

static Node *
mul(void)
{
	Node *np, *(*fun)(int, Node *, Node *);
	int op;

	np = cast(1);
	for (;;) {
		switch (yytoken) {
		case '*': op = OMUL; fun = arithmetic; break;
		case '/': op = ODIV; fun = arithmetic; break;
		case '%': op = OMOD; fun = integerop;  break;
		default: return np;
		}
		next();
		np = (*fun)(op, np, cast(1));
	}
}

static Node *
add(void)
{
	int op;
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
	int op;
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
	int op;
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
	int op;
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
		ifyes = xexpr();
		expect(':');
		ifno = ternary();
		np = chkternary(ifyes, ifno);
		cond = node(OASK, np->type, cond, np);
	}
	return cond;
}

static Node *
xassign(void)
{
	Node *np, *(*fun)(int , Node *, Node *);
	int op;

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
		chklvalue(np);
		np->flags |= NEFFECT;
		next();
		np = (fun)(op, np, assign());
	}
}

static Node *
xexpr(void)
{
	Node *lp, *rp;

	lp = xassign();
	while (accept(',')) {
		rp = xassign();
		lp = node(OCOMMA, rp->type, lp, rp);
	}
	return lp;
}

Node *
assign(void)
{
	return simplify(xassign());
}

Node *
constexpr(void)
{
	Node *np;

	np = ternary();
	if (np && np->type->op == INT) {
		np = simplify(convert(np, inttype, 0));
		if (np->flags & NCONST)
			return np;
	}
	freetree(np);
	return NULL;
}

Node *
expr(void)
{
	return simplify(xexpr());
}

Node *
condexpr(int neg)
{
	Node *np;

	np = exp2cond(xexpr(), neg);
	if (np->flags & NCONST)
		warn("conditional expression is constant");
	return simplify(np);
}
