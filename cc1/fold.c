/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc1/fold.c";
#include <stdio.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc1.h"


TUINT
ones(int nbytes)
{
	TUINT v;

	for (v = 0; nbytes--; v |= 255)
		v <<= 8;
	return v;
}

static int
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

static int
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

static int
subi(TINT l, TINT r, Type *tp)
{
	return addi(l, -r, tp);
}

static int
subf(TFLOAT l, TFLOAT r, Type *tp)
{
	return addf(l, -r, tp);
}

static int
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

static int
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

static int
divi(TINT l, TINT r,  Type *tp)
{
	struct limits *lim = getlimits(tp);

	if (r == 0 || l == -lim->min.i && r == -1) {
		warn("overflow in constant expression");
		return 0;
	}
	return 1;
}

static int
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

static int
lshi(TINT l, TINT r, Type *tp)
{
	if (r < 0 || r >= tp->size * 8) {
		warn("shifting %d bits is undefined", r);
		return 0;
	}
	return muli(l, 1 << r, tp);
}

static int
rshi(TINT l, TINT r, Type *tp)
{
	if (r < 0 || r >= tp->size * 8) {
		warn("shifting %d bits is undefined", r);
		return 0;
	}
	return 1;
}

static int
foldint(int op, Symbol *res, TINT l, TINT r)
{
	TINT i;
	Type *tp = res->type;
	int (*validate)(TINT, TINT, Type *tp);

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
	case ONEG:  i = !l;     break;
	case OSNEG: i = -l;     break;
	case OCPL:  i = ~l;     break;
	default:    return 0;
	}
	res->u.i = i;

	DBG("FOLD i l=%lld %d r=%lld = %lld", l, op, r, i);
	return 1;
}

static int
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
	case ONEG:  u = !l;     break;
	case OSNEG: u = -l;     break;
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

	DBG("FOLD ui l=%llu %d r=%llu = %llu", l, op, r, u);
	return 1;

sign:
	res->u.i = i;

	DBG("FOLD sui %llu %d %llu = %llu", l, op, r, i);
	return 1;
}

static int
foldfloat(int op, Symbol *res, TFLOAT l, TFLOAT r)
{
	TFLOAT f;
	TINT i;
	int (*validate)(TFLOAT, TFLOAT, Type *tp);

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
	case OLT:  i = l < r;  goto comparison;
	case OGT:  i = l > r;  goto comparison;
	case OGE:  i = l >= r; goto comparison;
	case OLE:  i = l <= r; goto comparison;
	case OEQ:  i = l == r; goto comparison;
	case ONE:  i = l != r; goto comparison;
	default:   return 0;
	}
	res->u.f = f;

	DBG("FOLD f l=%lf %d r=%lf = %lf", l, op, r, f);
	return 1;

comparison:
	res->u.i = i;

	DBG("FOLD if l=%lf %d r=%lf = %lld", l, op, r, i);
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
	sym = newsym(NS_IDEN, NULL);
	sym->flags |= SCONSTANT;
	sym->type = tp;
	sym->u = aux.u;
	return constnode(sym);
}

static Node *
foldcast(Node *np, Node *l)
{
	TUINT negmask, mask, u;
	Type *newtp = np->type, *oldtp = l->type;
	Symbol aux, *sym, *osym = l->sym;

	if (!(l->flags & NCONST))
		return np;

	switch (newtp->op) {
	case PTR:
	case INT:
	case ENUM:
		switch (oldtp->op) {
		case PTR:
		case INT:
		case ENUM:
			u = (oldtp->prop & TSIGNED) ? osym->u.i : osym->u.u;
			break;
		case FLOAT:
			oldtp = newtp;
			u = osym->u.f;
			break;
		default:
			return  np;
		}
		mask = ones(newtp->size);
		if (newtp->prop & TSIGNED) {
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
		aux.u.f = (oldtp->prop & TSIGNED) ? osym->u.i : osym->u.u;
		break;
	default:
		return np;
	}
	DBG("FOLD cast %c->%c", oldtp->letter, newtp->letter);
	freetree(np);
	sym = newsym(NS_IDEN, NULL);
	sym->flags |= SCONSTANT;
	sym->type = newtp;
	sym->u = aux.u;
	return constnode(sym);
}

static Node *
foldunary(Node *np, Node *l)
{
	int op = l->op;
	Node *aux;

	switch (np->op) {
	case ONEG:
		if (l->op == ONEG)
			break;
		return NULL;
	case OADD:
		DBG("FOLD unary delete %d", np->op);
		np->left = NULL;
		freetree(np);
		return l;
	case OCAST:
		if (op != OCAST)
			return foldcast(np, l);
		DBG("FOLD unary collapse %d", np->op);
		np->left = l->left;
		l->left = NULL;
		freetree(l);
		return np;
	case OSNEG:
	case OCPL:
		if (op != np->op)
			return NULL;
		break;
	case OPTR:
		if (op != OADDR || np->type != l->left->type)
			return NULL;
		break;
	case OADDR:
		if (op != OPTR)
			return NULL;
		break;
	default:
		return NULL;
	}
	DBG("FOLD unary cancel %d", np->op);
	aux = l->left;
	l->left = NULL;
	freetree(np);
	return aux;
}

static Node *
fold(Node *np)
{
	Symbol *rs, *ls;
	Type *optype;
	int type;
	int op = np->op;
	Node *p, *lp = np->left, *rp = np->right;
	Type *tp = np->type;

	if (!lp && !rp)
		return np;
	if (!rp && (p = foldunary(np, lp)) != NULL)
		return p;
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
	if (!rp) {
		rs = NULL;
	} else {
		if (!(rp->flags & NCONST) || !rp->sym)
			return NULL;
		rs = rp->sym;
	}

	if (!(lp->flags & NCONST) || !lp->sym)
		return NULL;
	optype = lp->type;
	ls = lp->sym;

	switch (type = optype->op) {
	case ENUM:
	case INT:
		if (!(optype->prop & TSIGNED))
			type = UNSIGNED;
	case PTR:
	case FLOAT:
		if ((p = foldconst(type, op, tp, ls, rs)) == NULL)
			return NULL;
		freetree(np);
		return p;
	default:
		return NULL;
	}
}

static void
commutative(Node *np, Node *l, Node *r)
{
	int op = np->op;

	if (r == NULL || r->flags&NCONST || !(l->flags&NCONST))
		return;

	switch (op) {
	case OLT:
	case OGT:
	case OGE:
	case OLE:
		DBG("FOLD neg commutative %d", np->op);
		np->op = negop(op);
	case OEQ:
	case ONE:
	case OADD:
	case OMUL:
	case OBAND:
	case OBXOR:
	case OBOR:
		DBG("FOLD commutative %d", np->op);
		np->left = r;
		np->right = l;
		break;
	}
}

static Node *
identity(Node *np)
{
	int iszeror, isoner;
	int iszerol, isonel;
	Node *lp = np->left, *rp = np->right;

	if (!rp)
		return NULL;

	iszeror = cmpnode(rp, 0);
	isoner = cmpnode(rp, 1),
	iszerol = cmpnode(lp, 0);
	isonel = cmpnode(lp, 1);

	switch (np->op) {
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
		/* TODO: i % 2^n => i & n-1 */
		if (isoner)
			goto change_to_comma;
	default:
		return NULL;
	}

free_right:
	DBG("FOLD identity %d", np->op);
	np->left = NULL;
	freetree(np);
	return lp;

free_left:
	DBG("FOLD identity %d", np->op);
	np->right = NULL;
	freetree(np);
	return rp;

change_to_comma:
	DBG("FOLD identity %d", np->op);
	np->op = OCOMMA;
	return np;
}

static Node *
foldternary(Node *np, Node *cond, Node *body)
{
	if (!(cond->flags & NCONST))
		return np;
	if (cmpnode(cond, 0)) {
		np = body->right;
		freetree(body->left);
	} else {
		np = body->left;
		freetree(body->right);
	}

	DBG("FOLD ternary");
	body->left = NULL;
	body->right = NULL;
	freetree(cond);
	free(body);
	return np;
}

/* TODO: fold OCOMMA */

Node *
simplify(Node *np)
{
	Node *p, *l, *r;

	if (!np)
		return NULL;
	if (debug)
		prtree(np);

	l = np->left = simplify(np->left);
	r = np->right = simplify(np->right);

	switch (np->op) {
	case OASK:
		return foldternary(np, l, r);
	case OCALL:
	case OPAR:
		return np;
	default:
		commutative(np, l, r);
		if ((p = fold(np)) != NULL)
			return p;
		if ((p = identity(np)) != NULL)
			return p;
		return np;
	}
}
