
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define NR_TYPE_HASH 16

Type
	*voidtype = &(Type) {
		.op = VOID,
		.letter = L_VOID
	},
	*pvoidtype = &(Type) {
		.op = PTR,
		.letter = L_POINTER
	},
	*booltype = &(Type) {
		.op = INT,
		.letter = L_BOOL,
		.defined = 1,
		.n.rank = RANK_BOOL
	},
	*schartype = &(Type) {
		.op = INT,
		.letter = L_SCHAR,
		.defined = 1,
		.n.rank = RANK_SCHAR
	},
	*uchartype = &(Type) {
		.op = INT,
		.letter = L_UCHAR,
		.sign = 1,
		.defined = 1,
		.n.rank = RANK_UCHAR
	},
	*chartype = &(Type) {
		.op = INT,
		.letter = L_CHAR,
		.sign = 1,
		.defined = 1,
		.n.rank = RANK_CHAR
	},
	*ushortype = &(Type) {
		.op = INT,
		.letter = L_USHORT,
		.defined = 1,
		.n.rank = RANK_USHORT
	},
	*shortype = &(Type) {
		.op = INT,
		.letter = L_SHORT,
		.defined = 1,
		.n.rank = RANK_SHORT
	},
	*uinttype = &(Type) {
		.op = INT,
		.letter = L_UINT,
		.sign = 1,
		.defined = 1,
		.n.rank = RANK_UINT
	},
	*inttype = &(Type) {
		.op = INT,
		.letter = L_INT,
		.defined = 1,
		.n.rank = RANK_INT
	},
	*longtype = &(Type) {
		.op = INT,
		.letter = L_LONG,
		.defined = 1,
		.n.rank = RANK_LONG
	},
	*ulongtype = &(Type) {
		.op = INT,
		.letter = L_ULONG,
		.sign = 1,
		.defined = 1,
		.n.rank = RANK_ULONG
	},
	*ullongtype = &(Type) {
		.op = INT,
		.letter = L_ULLONG,
		.sign = 1,
		.defined = 1,
		.n.rank = RANK_ULLONG
	},
	*llongtype = &(Type) {
		.op = INT,
		.letter = L_LLONG,
		.defined = 1,
		.n.rank = RANK_LLONG
	},
	*floattype = &(Type) {
		.op = FLOAT,
		.letter = L_FLOAT,
		.defined = 1,
		.n.rank = RANK_FLOAT
	},
	*doubletype = &(Type) {
		.op = FLOAT,
		.letter = L_DOUBLE,
		.defined = 1,
		.n.rank = RANK_DOUBLE
	},
	*ldoubletype = &(Type) {
		.op = FLOAT,
		.letter = L_LDOUBLE,
		.defined = 1,
		.n.rank = RANK_LDOUBLE
	};

Type *
ctype(int8_t type, int8_t sign, int8_t size)
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
		if (!sign && !size) {
			warn(options.implicit,
			     "type defaults to 'int' in declaration");
		}
		/* fallthrough */
	case INT:
		switch (size) {
		case 0:
			return (sign == UNSIGNED) ? uinttype   : inttype;
		case SHORT:
			return (sign == UNSIGNED) ? ushortype  : shortype;
		case LONG:
			return (sign == UNSIGNED) ? ulongtype  : longtype;
		case LONG+LONG:
			return (sign == UNSIGNED) ? ullongtype : llongtype;
		}
		break;
	case DOUBLE:
		if (size == LONG+LONG)
			goto invalid_type;
		size += LONG;
		goto floating;
	case FLOAT:
		if (size == LONG+LONG)
			goto invalid_type;
	floating:
		if (sign)
			goto invalid_type;
		switch (size) {
		case 0:
			return floattype;
		case LONG:
			return doubletype;
		case LONG+LONG:
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
mktype(Type *tp, uint8_t op, short nelem, void *data)
{
	static Type *typetab[NR_TYPE_HASH], **tbl, type;
	static uint8_t t;
	Type *bp;
	static char letters[] = {
		[PTR] = L_POINTER,   [ARY] = L_ARRAY,
		[FTN] = L_FUNCTION,  [ENUM] = L_INT,
		[STRUCT] = L_STRUCT, [UNION] = L_UNION
	};

	if (op == PTR && tp == voidtype)
		return pvoidtype;

	type.type = tp;
	type.op = op;
	type.sign = 0;
	type.letter = letters[op];
	type.pars = data;
	type.n.elem = nelem;
	type.ns = 0;

	if (op == ARY && nelem == 0 || op == STRUCT || op == UNION)
		type.defined = 0;
	else
		type.defined = 1;

	t = (op ^ (uint8_t) ((unsigned short) tp >> 3)) & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	for (bp = *tbl; bp; bp = bp->next) {
		if (eqtype(bp, &type)) {
			free(data);
			return bp;
		}
	}

	bp = xmalloc(sizeof(*bp));
	*bp = type;
	bp->next = *tbl;
	return *tbl = bp;
}

bool
eqtype(Type *tp1, Type *tp2)
{
	uint8_t n;
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
		/* TODO: Check when two enum are the same type */
	case INT: case FLOAT:
		return tp1->letter == tp2->letter;
	default:
		fputs("internal type error, aborting\n", stderr);
		abort();
	}
}
