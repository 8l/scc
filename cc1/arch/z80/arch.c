
#include <stdio.h>

#include "arch.h"
#include "../../../inc/cc.h"
#include "../../cc1.h"

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
		.printed = 1,
		.defined = 1,
	},
	{      /* 2 = booltype */
		.op = INT,
		.letter = L_BOOL,
		.defined = 1,
		.size = 1,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_BOOL,
		.printed = 1
	},
	{       /* 3 = schartype */
		.op = INT,
		.letter = L_INT8,
		.defined = 1,
		.size = 1,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_SCHAR,
		.printed = 1
	},
	{      /* 4 = uchartype */
		.op = INT,
		.letter = L_UINT8,
		.defined = 1,
		.size = 1,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_UCHAR,
		.printed = 1
	},
	{      /* 5 = chartype */
		.op = INT,
		.letter = L_UINT8,
		.defined = 1,
		.size = 1,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_CHAR,
		.printed = 1
	},
	{       /* 6 = ushortype */
		.op = INT,
		.letter = L_UINT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_USHORT,
		.printed = 1
	},
	{       /* 7 = shortype */
		.op = INT,
		.letter = L_INT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_SHORT,
		.printed = 1
	},
	{       /* 8 = uinttype */
		.op = INT,
		.letter = L_UINT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_UINT,
		.printed = 1
	},
	{       /* 9 = inttype */
		.op = INT,
		.letter = L_INT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_INT,
		.printed = 1
	},
	{      /* 10 = longtype */
		.op = INT,
		.letter = L_INT32,
		.defined = 1,
		.size = 4,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_LONG,
		.printed = 1
	},
	{       /* 11 = ulongtype */
		.op = INT,
		.letter = L_UINT32,
		.defined = 1,
		.size = 4,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_ULONG,
		.printed = 1
	},
	{	/* 12 = ullongtype */
		.op = INT,
		.letter = L_UINT64,
		.defined = 1,
		.size = 8,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_ULLONG,
		.printed = 1
	},
	{       /* 13 = llongtype */
		.op = INT,
		.letter = L_INT64,
		.defined = 1,
		.size = 8,
		.integer = 1,
		.arith = 1,
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
		.arith = 1,
		.align = 1,
		.n.rank = RANK_FLOAT,
		.printed = 1
	},
	{       /* 15 = doubletype */
		.op = FLOAT,
		.letter = L_DOUBLE,
		.defined = 1,
		.size = 8,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_DOUBLE,
		.printed = 1
	},
	{       /* 16 = ldoubletype */
		.op = FLOAT,
		.letter = L_LDOUBLE,
		.defined = 1,
		.size = 16,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_LDOUBLE,
		.printed = 1
	},
	{       /* 17 = sizettype */
		.op = INT,
		.letter = L_UINT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.n.rank = RANK_UINT,
		.printed = 1
	},
	{      /* 18 = ellipsis */
		.op = ELLIPSIS,
		.letter = L_ELLIPSIS,
		.defined = 1,
		.printed = 1
	},
	{       /* 7 = pdifftype */
		.op = INT,
		.letter = L_INT16,
		.defined = 1,
		.size = 2,
		.integer = 1,
		.arith = 1,
		.align = 1,
		.sign = 1,
		.n.rank = RANK_SHORT,
		.printed = 1
	},
};

Type *voidtype = &types[0], *pvoidtype = &types[1],
     *booltype = &types[2], *schartype = &types[3],
     *uchartype = &types[4], *chartype = &types[5],
     *ushortype = &types[6], *shortype = &types[7],
     *uinttype = &types[8], *inttype = &types[9],
     *longtype = &types[10], *ulongtype = &types[11],
     *ullongtype = &types[12], *llongtype = &types[13],
     *floattype = &types[14], *doubletype = &types[15],
     *ldoubletype = &types[16],
     *sizettype = &types[17], *pdifftype = &types[19],
     *ellipsistype = &types[18];

static Symbol dummy0 = {.u.i = 0, .type = &types[9]},
              dummy1 = {.u.i = 1, .type = &types[9]};
Symbol *zero = &dummy0, *one = &dummy1;
