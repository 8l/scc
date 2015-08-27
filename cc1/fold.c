
#include <stdio.h>

#include "../inc/cc.h"
#include "cc1.h"


#define SYMICMP(sym, val) (((sym)->type->sign) ?         \
	(sym)->u.i == (val) : (sym)->u.u == (val))

#define FOLDINT(sym, ls, rs, op) (((sym)->type->sign) ? \
	((sym)->u.i = ((ls)->u.i op (rs)->u.i)) :       \
	((sym)->u.u = ((ls)->u.u op (rs)->u.u)))

#define CMPISYM(sym, ls, rs, op) (((sym)->type->sign) ? \
	((ls)->u.i op (rs)->u.i) : ((ls)->u.u op (rs)->u.u))

Node *
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

Node *
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

/* TODO: check validity of types */

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
