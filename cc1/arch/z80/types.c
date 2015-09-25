
#include <stdio.h>

#include "../../../inc/cc.h"
#include "../../cc1.h"
#include "arch.h"

#define RANK_BOOL    0
#define RANK_SCHAR   1
#define RANK_UCHAR   2
#define RANK_CHAR    3
#define RANK_SHORT   4
#define RANK_USHORT  5
#define RANK_INT     6
#define RANK_UINT    7
#define RANK_LONG    8
#define RANK_ULONG   9
#define RANK_LLONG   10
#define RANK_ULLONG  11
#define RANK_FLOAT   12
#define RANK_DOUBLE  13
#define RANK_LDOUBLE 15

#define L_SCHAR     L_INT8
#define L_UCHAR     L_UINT8
#define L_CHAR      L_UINT8
#define L_SHORT     L_INT16
#define L_USHORT    L_UINT16
#define L_UINT      L_UINT16
#define L_LONG      L_INT32
#define L_ULONG     L_UINT32
#define L_LLONG     L_INT64
#define L_ULLONG    L_UINT64
#define L_BOOL      'B'
#define L_FLOAT     'J'
#define L_DOUBLE    'D'
#define L_LDOUBLE   'H'

/*
 * Compiler can generate warnings here if the ranges of TINT,
 * TUINT and TFLOAT are smaller than any of the constants in this
 * array. Ignore them if you know that the target types are correct
 */
struct limits limits[][4] = {
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
