
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define NR_TYPE_HASH 16

/*
 * Compiler can generate warnings here if the ranges of TINT,
 * TUINT and TFLOAT are smaller than any of the constants in this
 * array. Ignore them if you know that the target types are correct
 */
static struct limits limits[][4] = {
	{
		{	/* 0 = unsigned 1 byte */
			.min.u = 0,
			.max.u = 255
		},
		{	/* 1 = unsigned 2 bytes */
			.min.u = 0,
			.max.u = 65535u
		},
		{	/* 2 = unsigned 4 bytes */
			.min.u = 0,
			.max.u = 4294967295u
		},
		{	/* 3 = unsigned 4 bytes */
			.min.u = 0,
			.max.u = 18446744073709551615u
		}
	},
	{
		{	/* 0 = signed 1 byte */
			.min.i = -127,
			.max.i = 127
		},
		{	/* 1 = signed 2 byte */
			.min.i = -32767,
			.max.i = 32767
		},
		{	/* 2 = signed 4 byte */
			.min.i = -2147483647L,
			.max.i = 2147483647L
		},
		{	/* 3 = signed 8 byte */
			.min.i = -9223372036854775807LL,
			.max.i = 9223372036854775807LL,
		}
	},
	{
		{
			/* 0 = float 4 bytes */
			.min.f = -1,
			.max.f = 2
		},
		{
			/* 1 = float 8 bytes */
			.min.f = -1,
			.max.f = 2,
		},
		{
			/* 2 = float 16 bytes */
			.min.f = -1,
			.max.f = 2,
		}
	}
};

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
	type.printed = 0;
	type.letter = letters[op];
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
