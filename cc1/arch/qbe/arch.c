/* See LICENSE file for copyright and license details. */
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
		.prop = TPRINTED,
	},
	{       /* 1 = pvoidtype */
		.op = PTR,
		.letter = L_POINTER,
		.prop = TDEFINED | TPRINTED,
		.size = 8,
		.align = 8,
	},
	{      /* 2 = booltype */
		.op = INT,
		.letter = L_BOOL,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 1,
		.align = 1,
		.n.rank = RANK_BOOL,
	},
	{       /* 3 = schartype */
		.op = INT,
		.letter = L_INT8,
		.prop = TDEFINED | TINTEGER | TARITH | SIGNED | TPRINTED,
		.size = 1,
		.align = 1,
		.n.rank = RANK_SCHAR,
	},
	{      /* 4 = uchartype */
		.op = INT,
		.letter = L_UINT8,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 1,
		.align = 1,
		.n.rank = RANK_UCHAR,
	},
	{      /* 5 = chartype */
		.op = INT,
		.letter = L_INT8,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 1,
		.align = 1,
		.n.rank = RANK_CHAR,
	},
	{       /* 6 = ushortype */
		.op = INT,
		.letter = L_UINT16,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 2,
		.align = 2,
		.n.rank = RANK_USHORT,
	},
	{       /* 7 = shortype */
		.op = INT,
		.letter = L_INT16,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 2,
		.align = 2,
		.n.rank = RANK_SHORT,
	},
	{       /* 8 = uinttype */
		.op = INT,
		.letter = L_UINT32,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 4,
		.align = 4,
		.n.rank = RANK_UINT,
	},
	{       /* 9 = inttype */
		.op = INT,
		.letter = L_INT32,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 4,
		.align = 4,
		.n.rank = RANK_INT,
	},
	{      /* 10 = longtype */
		.op = INT,
		.letter = L_INT64,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_LONG,
	},
	{       /* 11 = ulongtype */
		.op = INT,
		.letter = L_UINT64,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_ULONG,
	},
	{	/* 12 = ullongtype */
		.op = INT,
		.letter = L_UINT64,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_ULLONG,
	},
	{       /* 13 = llongtype */
		.op = INT,
		.letter = L_INT64,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_LLONG,
	},
	{       /* 14 = floattype */
		.op = FLOAT,
		.letter = L_FLOAT,
		.prop = TDEFINED | TARITH | TPRINTED,
		.size = 4,
		.align = 4,
		.n.rank = RANK_FLOAT,
	},
	{       /* 15 = doubletype */
		.op = FLOAT,
		.letter = L_DOUBLE,
		.prop = TDEFINED | TARITH | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_DOUBLE,
	},
	{       /* 16 = ldoubletype */
		.op = FLOAT,
		.letter = L_LDOUBLE,
		.prop = TDEFINED | TARITH | TPRINTED,
		.size = 16,
		.align = 16,
		.n.rank = RANK_LDOUBLE,
	},
	{       /* 17 = sizettype */
		.op = INT,
		.letter = L_UINT32,
		.prop = TDEFINED | TINTEGER | TARITH | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_UINT,
	},
	{      /* 18 = ellipsis */
		.op = ELLIPSIS,
		.letter = L_ELLIPSIS,
		.prop = TDEFINED | TPRINTED,
	},
	{      /* 19 = pdifftype */
		.op = INT,
		.letter = L_INT64,
		.prop = TDEFINED | TINTEGER | TARITH | TSIGNED | TPRINTED,
		.size = 8,
		.align = 8,
		.n.rank = RANK_LONG,
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
