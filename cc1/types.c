
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sizes.h>
#include <cc.h>
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
		.u.rank = RANK_BOOL
	},
	*schartype = &(Type) {
		.op = INT,
		.letter = L_SCHAR,
		.defined = 1,
		.u.rank = RANK_SCHAR
	},
	*uchartype = &(Type) {
		.op = INT,
		.letter = L_UCHAR,
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_UCHAR
	},
	*chartype = &(Type) {
		.op = INT,
		.letter = L_CHAR,
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_CHAR
	},
	*ushortype = &(Type) {
		.op = INT,
		.letter = L_USHORT,
		.defined = 1,
		.u.rank = RANK_USHORT
	},
	*shortype = &(Type) {
		.op = INT,
		.letter = L_SHORT,
		.defined = 1,
		.u.rank = RANK_SHORT
	},
	*uinttype = &(Type) {
		.op = INT,
		.letter = L_UINT,
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_UINT
	},
	*inttype = &(Type) {
		.op = INT,
		.letter = L_INT,
		.defined = 1,
		.u.rank = RANK_INT
	},
	*longtype = &(Type) {
		.op = INT,
		.letter = L_LONG,
		.defined = 1,
		.u.rank = RANK_LONG
	},
	*ulongtype = &(Type) {
		.op = INT,
		.letter = L_ULONG,
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_ULONG
	},
	*ullongtype = &(Type) {
		.op = INT,
		.letter = L_ULLONG,
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_ULLONG
	},
	*llongtype = &(Type) {
		.op = INT,
		.letter = L_LLONG,
		.defined = 1,
		.u.rank = RANK_LLONG
	},
	*floattype = &(Type) {
		.op = FLOAT,
		.letter = L_FLOAT,
		.defined = 1,
		.u.rank = RANK_FLOAT
	},
	*doubletype = &(Type) {
		.op = FLOAT,
		.letter = L_DOUBLE,
		.defined = 1,
		.u.rank = RANK_DOUBLE
	},
	*ldoubletype = &(Type) {
		.op = FLOAT,
		.letter = L_LDOUBLE,
		.defined = 1,
		.u.rank = RANK_LDOUBLE
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
mktype(Type *tp, uint8_t op, void *data)
{
	static Type *typetab[NR_TYPE_HASH], **tbl;
	static uint8_t t, def;
	register Type *bp;
	char letter, look;
	union typeval u;

	switch (op) {
	case PTR:
		if (tp == voidtype)
			return pvoidtype;
		letter = L_POINTER;
		def = 1;
		look = 1;
		break;
	case ARY:
		u.nelem = *(unsigned short *) data;
		letter = L_ARRAY;
		def = u.nelem != 0;
		look = 1;
		break;
	case FTN:
		u.pars = data;
		letter = L_FUNCTION;
		def = 1;
		look = 0;
		break;
	case ENUM:
		letter = L_INT;
		def = 1;
		look = 0;
		break;
	case STRUCT: case UNION:
		letter = (op == STRUCT) ? L_STRUCT : L_UNION;
		def = 0;
		look = 0;
		u.fields = NULL;
		break;
	default:
		fputs("internal type error, aborting\n", stderr);
		abort();
	}

	t = (op  ^  (uint8_t) ((unsigned short) tp >> 3))
	         & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	if (look) {
		for (bp = *tbl; bp; bp = bp->next) {
			if (bp->type == tp && bp->op == op &&
			    (op != ARY || bp->u.nelem == u.nelem)) {
				return bp;
			}
		}
	}

	bp = xcalloc(1, sizeof(*bp));
	bp->next = *tbl;
	bp->type = tp;
	bp->op = op;
	bp->letter = letter;
	bp->defined = def;
	bp->u = u;
	return *tbl = bp;
}

bool
eqtype(Type *tp1, Type *tp2)
{
	if (tp1 == tp2)
		return 1;
	if (tp1->op != tp2->op)
		return 0;
	switch (tp1->op) {
	case PTR:
		return eqtype(tp1->type, tp2->type);
	/* TODO: use the same struct for function parameters and fields */
	case FTN: {
		Funpar *fp1 = tp1->u.pars, *fp2 = tp2->u.pars;

		while (fp1 && fp2) {
			if (!eqtype(fp1->type, fp2->type))
				break;
			fp1 = fp1->next;
			fp2 = fp2->next;
		}
		return fp1 == fp2;
	}
	case UNION:
	case STRUCT: {
		Field *fp1 = tp1->u.fields, *fp2 = tp2->u.fields;

		while (fp1 && fp2) {
			if (!eqtype(fp1->type, fp2->type))
				break;
			fp1 = fp1->next;
			fp2 = fp2->next;
		}
		return fp1 == fp2;
	}
	case ARY:
		if (!eqtype(tp1->type, tp2->type))
			return 0;
		return tp1->u.nelem == tp2->u.nelem;
	case ENUM:
		return 1;
	default:
		fputs("internal type error, aborting\n", stderr);
		abort();
	}
}
