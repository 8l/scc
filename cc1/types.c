
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define NR_TYPE_HASH 16

/*
 * Initializaion of type pointers were done with
 * a C99 initilizator '... = &(Type) {...', but
 * c compiler in Plan9 gives error with this
 * syntax, so I have switched it to this ugly form
 * I hope I will change it again in the future
 */
static Type types[] = {
	{       /* 0 = voidtype */
		.op = VOID,
		.letter = L_VOID,
		.printed = 1
	},
	{       /* 1 = pvoidtype */
		.op = PTR,
		.letter = L_POINTER,
		.size = 2,
		.align = 2,
		.printed = 1
	},
	{      /* 2 = booltype */
		.op = INT,
		.letter = L_BOOL,
		.defined = 1,
		.size = 1,
		.align = 1,
		.n.rank = RANK_BOOL,
		.printed = 1
	},
	{       /* 3 = schartype */
		.op = INT,
		.letter = L_SCHAR,
		.defined = 1,
		.size = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_SCHAR,
		.printed = 1
	},
	{      /* 4 = uchartype */
		.op = INT,
		.letter = L_UCHAR,
		.defined = 1,
		.size = 1,
		.align = 1,
		.n.rank = RANK_UCHAR,
		.printed = 1
	},
	{      /* 5 = chartype */
		.op = INT,
		.letter = L_CHAR,
		.defined = 1,
		.size = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_CHAR,
		.printed = 1
	},
	{       /* 6 = ushortype */
		.op = INT,
		.letter = L_USHORT,
		.defined = 1,
		.size = 2,
		.align = 1,
		.n.rank = RANK_USHORT,
		.printed = 1
	},
	{       /* 7 = shortype */
		.op = INT,
		.letter = L_SHORT,
		.defined = 1,
		.size = 2,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_SHORT,
		.printed = 1
	},
	{       /* 8 = uinttype */
		.op = INT,
		.letter = L_UINT,
		.defined = 1,
		.size = 2,
		.align = 1,
		.n.rank = RANK_UINT,
		.printed = 1
	},
	{       /* 9 = inttype */
		.op = INT,
		.letter = L_INT,
		.defined = 1,
		.size = 2,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_INT,
		.printed = 1
	},
	{      /* 10 = longtype */
		.op = INT,
		.letter = L_LONG,
		.defined = 1,
		.size = 4,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_LONG,
		.printed = 1
	},
	{       /* 11 = ulongtype */
		.op = INT,
		.letter = L_ULONG,
		.defined = 1,
		.size = 4,
		.align = 1,
		.n.rank = RANK_ULONG,
		.printed = 1
	},
	{	/* 12 = ullongtype */
		.op = INT,
		.letter = L_ULLONG,
		.defined = 1,
		.size = 8,
		.align = 1,
		.n.rank = RANK_ULLONG,
		.printed = 1
	},
	{       /* 13 = llongtype */
		.op = INT,
		.letter = L_LLONG,
		.defined = 1,
		.size = 8,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_LLONG,
		.printed = 1
	},
	{       /* 14 = floattype */
		.op = FLOAT,
		.letter = L_FLOAT,
		.defined = 1,
		.size = 4,
		.align = 1,
		.n.rank = RANK_FLOAT,
		.printed = 1
	},
	{       /* 15 = doubletype */
		.op = FLOAT,
		.letter = L_DOUBLE,
		.defined = 1,
		.size = 8,
		.align = 1,
		.n.rank = RANK_DOUBLE,
		.printed = 1
	},
	{       /* 16 = ldoubletype */
		.op = FLOAT,
		.letter = L_LDOUBLE,
		.defined = 1,
		.size = 16,
		.align = 1,
		.n.rank = RANK_LDOUBLE,
		.printed = 1
	},
	{       /* 17 = sizettype */
		.op = INT,
		.letter = L_UINT,
		.defined = 1,
		.size = 2,
		.align = 1,
		.n.rank = RANK_UINT,
		.printed = 1
	}
};

Type *voidtype = &types[0], *pvoidtype = &types[1],
	*booltype = &types[2], *schartype = &types[3],
	*uchartype = &types[4], *chartype = &types[5],
	*ushortype = &types[6], *shortype = &types[7],
	*uinttype = &types[8], *inttype = &types[9],
	*longtype = &types[10], *ulongtype = &types[11],
	*ullongtype = &types[12], *llongtype = &types[13],
	*floattype = &types[14], *doubletype = &types[15],
	*ldoubletype = &types[16], *sizettype = &types[17];

static Symbol dummy0 = {.u.i = 0, .type = &types[9]},
              dummy1 = {.u.i = 1, .type = &types[9]};
Symbol *zero = &dummy0, *one = &dummy1;


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
		warn("type defaults to 'int' in declaration");
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
	fputs("internal type error, aborting\n", stderr);
	abort();

invalid_type:
	error("invalid type specification");
}

Type *
mktype(Type *tp, unsigned op, short nelem, void *data)
{
	static Type *typetab[NR_TYPE_HASH];
	Type **tbl, type;
	unsigned t;
	Type *bp;
	static char letters[] = {
		[PTR] = L_POINTER,
		[ARY] = L_ARRAY,
		[FTN] = L_FUNCTION,
		[ENUM] = L_INT,
		[STRUCT] = L_STRUCT,
		[UNION] = L_UNION
	};

	if (op == PTR && tp == voidtype)
		return pvoidtype;

	type.type = tp;
	type.op = op;
	type.letter = letters[op];
	type.pars = data;
	type.n.elem = nelem;
	type.ns = 0;


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
		if (eqtype(bp, &type)) {
			/* FIXME: data can be a pointer to static data */
			free(data);
			return bp;
		}
	}

	bp = duptype(&type);
	bp->next = *tbl;
	return *tbl = bp;
}

bool
eqtype(Type *tp1, Type *tp2)
{
	unsigned n;
	Type **p1, **p2;

	if (tp1 == tp2)
		return 1;
	if (tp1->op != tp2->op || tp1->n.elem != tp2->n.elem)
		return 0;
	switch (tp1->op) {
	case ARY:
	case PTR:
		return eqtype(tp1->type, tp2->type);
	case UNION:
	case STRUCT:
	case FTN:
		p1 = tp1->pars, p2 = tp2->pars;
		for (n = tp1->n.elem; n != 0; --n) {
			if (!eqtype(*p1++, *p2++))
				return 0;
		}
		return 1;
	case ENUM:
		break;
	case INT: case FLOAT:
		return tp1->letter == tp2->letter;
	default:
		fputs("internal type error, aborting\n", stderr);
		abort();
	}
}
