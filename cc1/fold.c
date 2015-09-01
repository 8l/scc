
#include <stdio.h>

#include "../inc/cc.h"
#include "cc1.h"

static bool
addi(TINT l, TINT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TINT max = lim->max.i, min = lim->min.i;

	if (l < 0 && r < 0 && l >= min - r ||
	    l == 0 ||
	    r == 0 ||
	    l < 0 && r > 0 ||
	    l > 0 && r < 0 ||
	    l > 0 && r > 0 && l <= max - r) {
		return 1;
	}
	warn("overflow in constant expression");
	return 0;
}

static bool
subi(TINT l, TINT r, Type *tp)
{
	return addi(l, -r, tp);
}

static bool
muli(TINT l, TINT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TINT max = lim->max.i, min = lim->min.i;

	if (l > -1 && l <= 1 ||
	    r > -1 && r <= 1 ||
	    l < 0 && r < 0 && -l <= max/-r ||
	    l < 0 && r > 0 &&  l >= min/r  ||
	    l > 0 && r < 0 &&  r >= min/l  ||
	    l > 0 && r > 0 &&  l <= max/r) {
			return 1;
	}
	warn("overflow in constant expression");
	return 0;
}

static bool
divi(TINT l, TINT r,  Type *tp)
{
	struct limits *lim = getlimits(tp);

	if (r == 0) {
		warn("division by 0");
		return 0;
	}
	if (l == lim->min.i && r == -1) {
		warn("overflow in constant expression");
		return 0;
	}
	return 1;
}

static bool
lshi(TINT l, TINT r, Type *tp)
{
	if (r < 0 || r >= tp->size * 8) {
		warn("shifting %d bits is undefined", r);
		return 0;
	}
	return muli(l, 1 << r, tp);
}

static bool
rshi(TINT l, TINT r, Type *tp)
{
	if (r < 0 || r >= tp->size * 8) {
		warn("shifting %d bits is undefined", r);
		return 0;
	}
	return 1;
}

static bool
foldint(int op, Symbol *res, TINT l, TINT r)
{
	TINT i;
	bool (*validate)(TINT, TINT, Type *tp);

	switch (op) {
	case OADD: validate = addi; break;
	case OSUB: validate = subi; break;
	case OMUL: validate = muli; break;
	case ODIV: validate = divi; break;
	case OSHL: validate = lshi; break;
	case OSHR: validate = rshi; break;
	case OMOD: validate = divi; break;
	default:   validate = NULL; break;
	}

	if (validate && !(*validate)(l, r, res->type))
		return 0;

	switch (op) {
	case OADD:  i = l + r;  break;
	case OSUB:  i = l - r;  break;
	case OMUL:  i = l * r;  break;
	case ODIV:  i = l / r;  break;
	case OMOD:  i = l % r;  break;
	case OSHL:  i = l << r; break;
	case OSHR:  i = l >> r; break;
	case OBAND: i = l & r;  break;
	case OBXOR: i = l ^ r;  break;
	case OBOR:  i = l | r;  break;
	case OAND:  i = l && r; break;
	case OOR:   i = l || r; break;
	case OLT:   i = l < r;  break;
	case OGT:   i = l > r;  break;
	case OGE:   i = l >= r; break;
	case OLE:   i = l <= r; break;
	case OEQ:   i = l == r; break;
	case ONE:   i = l != r; break;
	case ONEG:  i = -l;     break;
	case OCPL:  i = ~l;     break;
	}
	res->u.i = i;
	return 1;
}

static bool
folduint(int op, Symbol *res, TINT l, TINT r)
{
	TINT i;
	TUINT u;

	switch (op) {
	case OADD:  u = l + r;  break;
	case OSUB:  u = l - r;  break;
	case OMUL:  u = l * r;  break;
	case ODIV:  u = l / r;  break;
	case OMOD:  u = l % r;  break;
	case OSHL:  u = l << r; break;
	case OSHR:  u = l >> r; break;
	case OBAND: u = l & r;  break;
	case OBXOR: u = l ^ r;  break;
	case OBOR:  u = l | r;  break;
	case ONEG:  u = -l;     break;
	case OCPL:  u = ~l;     break;
	case OAND:  i = l && r; goto unsign;
	case OOR:   i = l || r; goto unsign;
	case OLT:   i = l < r;  goto unsign;
	case OGT:   i = l > r;  goto unsign;
	case OGE:   i = l >= r; goto unsign;
	case OLE:   i = l <= r; goto unsign;
	case OEQ:   i = l == r; goto unsign;
	case ONE:   i = l != r; goto unsign;
	}

sign:
	res->u.u = u;
	return 1;

unsign:
	res->u.i = i;
	return 1;
}

static bool
foldfloat(int op, Symbol *res, TFLOAT l, TFLOAT r)
{
	TFLOAT f;
	TINT i;

	switch (op) {
	case OADD: f = l + r;  break;
	case OSUB: f = l - r;  break;
	case OMUL: f = l * r;  break;
	case ODIV: f = l / r;  break;
	case OLT:  i = l < r;  goto comparision;
	case OGT:  i = l > r;  goto comparision;
	case OGE:  i = l >= r; goto comparision;
	case OLE:  i = l <= r; goto comparision;
	case OEQ:  i = l == r; goto comparision;
	case ONE:  i = l != r; goto comparision;
	default:   return 0;
	}
	res->u.f = f;
	return 1;

comparision:
	res->u.i = i;
	return 1;
}

static Node *
foldconst(int type, int op, Type *tp, Symbol *ls, Symbol *rs)
{
	Symbol *sym, aux;
	TINT i;
	TUINT u;
	TFLOAT f;

	aux.type = ls->type;
	switch (type) {
	case INT:
		i = (rs) ? rs->u.i : 0;
		if (!foldint(op, &aux, ls->u.i, i))
			return NULL;
		break;
	case UNSIGNED:
		u = (rs) ? rs->u.u : 0u;
		if (!folduint(op, &aux, ls->u.u, u))
			return NULL;
		break;
	case FLOAT:
		f = (rs) ? rs->u.f : 0.0;
		if (!foldfloat(op, &aux, ls->u.f, f))
			return NULL;
		break;
	}
	sym = newsym(NS_IDEN);
	sym->type = tp;
	sym->u = aux.u;
	return constnode(sym);
}

static Node *
fold(int op, Type *tp, Node *lp, Node *rp)
{
	Symbol *rs, *ls;
	Node *np;
	int type;

	if (!lp->constant || rp && !rp->constant)
		return NULL;
	ls = lp->sym;
	rs = (rp) ? rp->sym : NULL;

	/*
	 * Comparision nodes have integer type
	 * but the operands can have different
	 * type.
	 */
	type = (isnodecmp(op)) ? BTYPE(lp) : tp->op;
	switch (type) {
	case PTR:
	case INT:
		type = (tp->sign) ? INT : UNSIGNED;
		break;
	case FLOAT:
		type = FLOAT;
		break;
	default:
		return NULL;
	}

	if ((np = foldconst(type, op, tp, ls, rs)) == NULL)
		return NULL;

	freetree(lp);
	freetree(rp);
	return np;
}

static void
commutative(int *op, Node **lp, Node **rp)
{
	Node *l = *lp, *r = *rp, *aux;

	if (r == NULL || r->constant || !l->constant)
		return;

	switch (*op) {
	case OLT:
	case OGT:
	case OGE:
	case OLE:
	case OEQ:
	case ONE:
		*op = negop(*op);
	case OADD:
	case OMUL:
	case OBAND:
	case OBXOR:
	case OBOR:
		aux = l;
		l = r;
		r = aux;
		*rp = r;
		*lp = l;
		break;
	default:
		return;
	}
}

static bool
cmp(Node *np, int val)
{
	Symbol *sym;
	Type *tp;

	if (!np->constant)
		return 0;
	sym = np->sym;
	tp = sym->type;

	switch (tp->op) {
	case PTR:
	case INT:
		return ((tp->sign) ? sym->u.i : sym->u.u) == val;
	case FLOAT:
		return sym->u.f == val;
	}
	return 0;
}

static TUINT
ones(int n)
{
	TUINT v;

	for (v = 1; n--; v |= 1)
		v <<= 1;
	return v;
}

static bool
identity(int op, Node *lp, Node *rp)
{
	int val;

	switch (op) {
	case OSHL:
	case OSHR:
	case OBXOR:
	case OADD:
	case OSUB:
		val = 0;
		break;
	case ODIV:
	case OMOD:
	case OMUL:
	case OBOR:
		val = 1;
		break;
	case OBAND:
		if (cmp(lp, ones(lp->type->size * 8)))
			goto free_right;
	default:
		return 0;
	}
	if (!cmp(rp, val))
		return 0;
free_right:
	freetree(rp);
	return 1;
}

Node *
simplify(int op, Type *tp, Node *lp, Node *rp)
{
	Node *np;

	if ((np = fold(op, tp, lp, rp)) != NULL)
		return np;
	commutative(&op, &lp, &rp);
	if (identity(op, lp, rp))
		return lp;
	return node(op, tp, lp, rp);
}

/* TODO: check validity of types */
/* TODO: Integrate it with simplify */

Node *
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
