
#include <stdio.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"


TUINT
ones(int nbytes)
{
	TUINT v;

	for (v = 0; nbytes--; v |= 255)
		v <<= 8;
	return v;
}

static bool
addi(TINT l, TINT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TINT max = lim->max.i, min = -lim->min.i;

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
addf(TFLOAT l, TFLOAT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TFLOAT max = lim->max.f, min = lim->min.f;

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
subf(TFLOAT l, TFLOAT r, Type *tp)
{
	return addf(l, -r, tp);
}

static bool
muli(TINT l, TINT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TINT max = lim->max.i, min = -lim->min.i;

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
mulf(TFLOAT l, TFLOAT r, Type *tp)
{
	struct limits *lim = getlimits(tp);
	TFLOAT max = lim->max.f, min = lim->min.f;

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

	if (r == 0 || l == -lim->min.i && r == -1) {
		warn("overflow in constant expression");
		return 0;
	}
	return 1;
}

static bool
divf(TFLOAT l, TFLOAT r,  Type *tp)
{
	struct limits *lim = getlimits(tp);

	if (l < 0) l = -l;
	if (r < 0) r = -r;

	if (r == 0.0 || r < 1.0 && l > lim->max.f * r) {
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
	Type *tp = res->type;
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

	if (validate && !(*validate)(l, r, tp))
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
	default:    return 0;
	}
	res->u.i = i;
	return 1;
}

static bool
folduint(int op, Symbol *res, TUINT l, TUINT r)
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
	case OAND:  i = l && r; goto sign;
	case OOR:   i = l || r; goto sign;
	case OLT:   i = l < r;  goto sign;
	case OGT:   i = l > r;  goto sign;
	case OGE:   i = l >= r; goto sign;
	case OLE:   i = l <= r; goto sign;
	case OEQ:   i = l == r; goto sign;
	case ONE:   i = l != r; goto sign;
	default:    return 0;
	}

	res->u.u = u & ones(res->type->size);
	return 1;

sign:
	res->u.i = i;
	return 1;
}

static bool
foldfloat(int op, Symbol *res, TFLOAT l, TFLOAT r)
{
	TFLOAT f;
	TINT i;
	bool (*validate)(TFLOAT, TFLOAT, Type *tp);

	switch (op) {
	case OADD: validate = addf; break;
	case OSUB: validate = subf; break;
	case OMUL: validate = mulf; break;
	case ODIV: validate = divf; break;
	default:   validate = NULL; break;
	}

	if (validate && !(*validate)(l, r, res->type))
		return 0;

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

	aux.type = tp;
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
	Type *optype;
	int type;

	if ((op == ODIV || op == OMOD) && cmpnode(rp, 0)) {
		warn("division by 0");
		return NULL;
	}
	/*
	 * Return if any of the children is no constant,
	 * or it is a constant generated when
	 * the address of a static variable is taken
	 * (when we don't know the physical address so
	 * we cannot fold it)
	 */
	if (!(lp->flags & NCONST) || !lp->sym ||
	    rp && (!(rp->flags & NCONST) || !rp->sym)) {
		return NULL;
	}
	optype = lp->type;
	ls = lp->sym;
	rs = (rp) ? rp->sym : NULL;

	switch (type = optype->op) {
	case ENUM:
	case INT:
		if (!optype->sign)
			type = UNSIGNED;
	case PTR:
	case FLOAT:
		if ((np = foldconst(type, op, tp, ls, rs)) != NULL)
			break;
	default:
		return NULL;
	}

	freetree(lp);
	freetree(rp);
	return np;
}

static void
commutative(int *op, Node **lp, Node **rp)
{
	Node *l = *lp, *r = *rp, *aux;

	if (r == NULL || r->flags & NCONST || !(l->flags & NCONST))
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
		break;
	}
	*rp = r;
	*lp = l;
}

static Node *
identity(int *op, Node *lp, Node *rp)
{
	int iszeror, isoner, istruer;
	int iszerol, isonel, istruel;

	if (!rp)
		return NULL;

	iszeror = cmpnode(rp, 0);
	isoner = cmpnode(rp, 1),
	istruer = !iszeror && rp->flags & NCONST;
	iszerol = cmpnode(lp, 0);
	isonel = cmpnode(lp, 1),
	istruel = !iszerol && lp->flags & NCONST;

	switch (*op) {
	case OOR:
		/*
		 * 1 || i => 1    (free right)
		 * i || 0 => i    (free right)
		 * 0 || i => i    (free left)
		 * i || 1 => i,1  (comma)
		 */
		if (isonel | iszeror)
			goto free_right;
		if (iszerol)
			goto free_left;
		if (isoner)
			goto change_to_comma;
		return NULL;
	case OAND:
		/*
		 * 0 && i => 0    (free right)
		 * i && 1 => i    (free right)
		 * 1 && i => i    (free left)
		 * i && 0 => i,0  (comma)
		 */
		if (iszerol | isoner)
			goto free_right;
		if (isonel)
			goto free_left;
		if (iszeror)
			goto change_to_comma;
		return NULL;
	case OSHL:
	case OSHR:
		/*
		 * i >> 0 => i    (free right)
		 * i << 0 => i    (free right)
		 * 0 >> i => 0    (free right)
		 * 0 << i => 0    (free right)
		 */
		if (iszeror | iszerol)
			goto free_right;
		return NULL;
	case OBXOR:
	case OADD:
	case OBOR:
	case OSUB:
		/*
		 * i + 0  => i
		 * i - 0  => i
		 * i | 0  => i
		 * i ^ 0  => i
		 */
		if (iszeror)
			goto free_right;
		return NULL;
	case OMUL:
		/*
		 * i * 0  => i,0
		 * i * 1  => i
		 */
		if (iszeror)
			goto change_to_comma;
		if (isoner)
			goto free_right;
		return NULL;
	case ODIV:
		/* i / 1  => i */
		if (isoner)
			goto free_right;
		return NULL;
	case OBAND:
		/* i & ~0 => i */
		if (cmpnode(rp, -1))
			goto free_right;
		return NULL;
	case OMOD:
		/* i % 1  => i,1 */
		if (isoner)
			goto change_to_comma;
	default:
		return NULL;
	}

free_right:
	freetree(rp);
	return lp;

free_left:
	freetree(lp);
	return rp;

change_to_comma:
	*op = OCOMMA;
	return NULL;
}

static Node *
foldternary(int op, Type *tp, Node *cond, Node *body)
{
	Node *np;

	if (!(cond->flags & NCONST))
		return node(op, tp, cond, body);
	if (cmpnode(cond, 0)) {
		np = body->right;
		freetree(body->left);
	} else {
		np = body->left;
		freetree(body->right);
	}
	freetree(cond);
	free(body);
	return np;
}

/*
 * TODO: transform simplify in a recursivity
 * function, because we are losing optimization
 * chances
 */
Node *
simplify(int op, Type *tp, Node *lp, Node *rp)
{
	Node *np;

	if (op == OASK)
		return foldternary(op, tp, lp, rp);
	commutative(&op, &lp, &rp);
	if ((np = fold(op, tp, lp, rp)) != NULL)
		return np;
	if ((np = identity(&op, lp, rp)) != NULL)
		return np;
	return node(op, tp, lp, rp);
}

/* TODO: check validity of types */

Node *
castcode(Node *np, Type *newtp)
{
	TUINT negmask, mask, u;
	Type *oldtp = np->type;
	Symbol aux, *sym, *osym = np->sym;

	if (!(np->flags & NCONST))
		goto noconstant;

	switch (newtp->op) {
	case PTR:
	case INT:
	case ENUM:
		switch (oldtp->op) {
		case PTR:
		case INT:
		case ENUM:
			u = (oldtp->sign) ? osym->u.i : osym->u.u;
			break;
		case FLOAT:
			oldtp = newtp;
			u = osym->u.f;
			break;
		default:
			goto noconstant;
		}
		mask = ones(newtp->size);
		if (newtp->sign) {
			negmask = ~mask;
			if (u & (negmask >> 1) & mask)
				u |= negmask;
			aux.u.i = u;
		} else {
			aux.u.u = u & mask;
		}
		break;
	case FLOAT:
		/* FIXME: The cast can be from another float type */
		aux.u.f = (oldtp->sign) ? osym->u.i : osym->u.u;
		break;
	default:
		goto noconstant;
	}

	sym = newsym(NS_IDEN);
	np->type = sym->type = newtp;
	np->sym = sym;
	sym->u = aux.u;

	return np;

noconstant:
	return node(OCAST, newtp, np, NULL);
}
