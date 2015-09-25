
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"
#include "arch.h"

#define NR_TYPE_HASH 16

extern struct limits limits[][4];

struct limits *
getlimits(Type *tp)
{
	int ntable, ntype;

	switch (tp->op) {
	case INT:
		ntable = tp->sign;
		switch (tp->size) {
		case 1: ntype = 0; break;
		case 2: ntype = 1; break;
		case 4: ntype = 2; break;
		case 8: ntype = 3; break;
		}
		break;
	case FLOAT:
		ntable = 2;
		switch (tp->size) {
		case 4:  ntype = 0; break;
		case 8:  ntype = 1; break;
		case 16: ntype = 2; break;
		}
		break;
	}

	return &limits[ntable][ntype];
}

Type *
ctype(unsigned type, unsigned sign, unsigned size)
{
	switch (type) {
	case CHAR:
		if (size)
			goto invalid_type;
		switch (sign) {
		case 0:
			return chartype;
		case SIGNED:
			return schartype;
		case UNSIGNED:
			return uchartype;
		}
		break;
	case VOID:
		if (size || sign)
			goto invalid_type;
		return voidtype;
	case BOOL:
		if (size || sign)
			goto invalid_type;
		return booltype;
	case 0:
		/* fallthrough */
	case INT:
		switch (size) {
		case 0:
			return (sign == UNSIGNED) ? uinttype   : inttype;
		case SHORT:
			return (sign == UNSIGNED) ? ushortype  : shortype;
		case LONG:
			return (sign == UNSIGNED) ? ulongtype  : longtype;
		case LLONG:
			return (sign == UNSIGNED) ? ullongtype : llongtype;
		}
		break;
	case DOUBLE:
		if (size == LLONG)
			goto invalid_type;
		size += LONG;
		goto floating;
	case FLOAT:
		if (size == LLONG)
			goto invalid_type;
	floating:
		if (sign)
			goto invalid_type;
		switch (size) {
		case 0:
			return floattype;
		case LONG:
			return doubletype;
		case LLONG:
			return ldoubletype;
		}
		break;
	}

invalid_type:
	error("invalid type specification");
}

static TINT
typesize(Type *tp)
{
	Symbol **sp;
	TINT n, size, align;

	switch (tp->op) {
	case ARY:
		return tp->n.elem * tp->type->size;
	case PTR:
		return pvoidtype->size;
	case STRUCT:
		size = 0;
		n = tp->n.elem;
		for (sp = tp->p.fields; n--; ++sp) {
			tp = (*sp)->type;
			align = tp->align-1;
			size = size + align & ~align;
			size += tp->size;
		}
		/* TODO: Add aligment of the first field */
		return size;
	case UNION:
		size = 0;
		n = tp->n.elem;
		for (sp = tp->p.fields; n--; ++sp) {
			tp = (*sp)->type;
			if (tp->size > size)
				size = tp->size;
		}
		/* TODO: Add aligment of the worst field */
		return size;
	case ENUM:
		return inttype->size;
	}
	return 0;
}

Type *
mktype(Type *tp, int op, TINT nelem, Type *pars[])
{
	static Type *typetab[NR_TYPE_HASH];
	Type **tbl, type;
	unsigned t;
	Type *bp;
	int c;

	if (op == PTR && tp == voidtype)
		return pvoidtype;

	switch (op) {
	case PTR:     c = L_POINTER;  break;
	case ARY:     c = L_ARRAY;    break;
	case FTN:     c = L_FUNCTION; break;
	case ENUM:    c = L_ENUM;     break;
	case STRUCT:  c = L_STRUCT;   break;
	case UNION:   c = L_UNION;    break;
	}

	type.type = tp;
	type.op = op;
	type.printed = 0;
	type.letter = c;
	type.p.pars = pars;
	type.n.elem = nelem;
	type.ns = 0;
	/* TODO: Set aligment for new types */

	switch (op) {
	case ARY:
		if (nelem == 0)
			goto no_defined;
		/* PASSTROUGH */
	case FTN:
	case PTR:
		type.defined = 1;
		break;
	case ENUM:
		type.printed = 1;
		/* PASSTROUGH */
	case STRUCT:
	case UNION:
	no_defined:
		type.defined = 0;
		break;
	}

	t = (op ^ (uintptr_t) tp >> 3) & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	for (bp = *tbl; bp; bp = bp->next) {
		if (eqtype(bp, &type) && op != STRUCT && op != UNION) {
			/*
			 * pars was allocated by the caller
			 * but the type already exists, so
			 * we have to deallocted it
			 */
			free(pars);
			return bp;
		}
	}

	type.size = typesize(&type);
	bp = duptype(&type);
	bp->next = *tbl;
	return *tbl = bp;
}

bool
eqtype(Type *tp1, Type *tp2)
{
	TINT n;
	Type **p1, **p2;

	if (!tp1 || !tp2)
		return 0;
	if (tp1 == tp2)
		return 1;
	switch (tp1->op) {
	case ARY:
		if (tp1->op != tp2->op || tp1->n.elem != tp2->n.elem)
			return 0;
	case PTR:
		return eqtype(tp1->type, tp2->type);
	case UNION:
	case STRUCT:
	case FTN:
		if (tp1->op != tp2->op || tp1->n.elem != tp2->n.elem)
			return 0;
		p1 = tp1->p.pars, p2 = tp2->p.pars;
		for (n = tp1->n.elem; n > 0; --n) {
			if (!eqtype(*p1++, *p2++))
				return 0;
		}
		return 1;
	case VOID:
	case ENUM:
		return 0;
	case INT:
	case FLOAT:
		return tp1->letter == tp2->letter;
	}
}
