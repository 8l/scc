
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sizes.h>
#include <cc.h>
#include "cc1.h"

#define NR_TYPE_HASH 16

Type
	*voidtype = &(Type) {
		.op = VOID,
		.letter = 'W'
	},
	*pvoidtype = &(Type) {
		.op = PTR,
		.letter = 'R'
	},
	*booltype = &(Type) {
		.op = INT,
		.letter = 'B',
		.defined = 1,
		.u.rank = RANK_BOOL
	},
	*schartype = &(Type) {
		.op = INT,
		.letter = 'C',
		.defined = 1,
		.u.rank = RANK_SCHAR
	},
	*uchartype = &(Type) {
		.op = INT,
		.letter = 'M',
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_UCHAR
	},
	*chartype = &(Type) {
		.op = INT,
		.letter = 'M',
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_UCHAR
	},
	*ushortype = &(Type) {
		.op = INT,
		.letter = 'E',
		.defined = 1,
		.u.rank = RANK_USHORT
	},
	*shortype = &(Type) {
		.op = INT,
		.letter = 'K',
		.defined = 1,
		.u.rank = RANK_SHORT
	},
	*uinttype = &(Type) {
		.op = INT,
		.letter = 'U',
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_UINT
	},
	*inttype = &(Type) {
		.op = INT,
		.letter = 'I',
		.defined = 1,
		.u.rank = RANK_INT
	},
	*longtype = &(Type) {
		.op = INT,
		.letter = 'L',
		.defined = 1,
		.u.rank = RANK_LONG
	},
	*ulongtype = &(Type) {
		.op = INT,
		.letter = 'Z',
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_ULONG
	},
	*ullongtype = &(Type) {
		.op = INT,
		.letter = 'O',
		.sign = 1,
		.defined = 1,
		.u.rank = RANK_ULLONG
	},
	*llongtype = &(Type) {
		.op = INT,
		.letter = 'J',
		.defined = 1,
		.u.rank = RANK_LLONG
	},
	*floattype = &(Type) {
		.op = FLOAT,
		.letter = 'F',
		.defined = 1,
		.u.rank = RANK_FLOAT
	},
	*doubletype = &(Type) {
		.op = FLOAT,
		.letter = 'D',
		.defined = 1,
		.u.rank = RANK_DOUBLE
	},
	*ldoubletype = &(Type) {
		.op = FLOAT,
		.letter = 'H',
		.defined = 1,
		.u.rank = RANK_LDOUBLE
	};

Type *
ctype(int8_t type, int8_t sign, int8_t size)
{
	if (type == DOUBLE)
		type = FLOAT, size += LONG;

	switch (type) {
	case CHAR: if (sign == 0)
					return chartype;
				return (sign == UNSIGNED) ?  uchartype : schartype;
	case VOID:            return voidtype;
	case BOOL:            return booltype;
	case INT: switch (size) {
		case 0:          return (sign == UNSIGNED) ? uinttype   : inttype;
		case SHORT:      return (sign == UNSIGNED) ? ushortype  : shortype;
		case LONG:       return (sign == UNSIGNED) ? ulongtype  : longtype;
		case LONG+LONG:  return (sign == UNSIGNED) ? ullongtype : llongtype;
		}
	case FLOAT: switch (size) {
		case 0:          return floattype;
		case LONG:       return doubletype;
		case LONG+LONG:  return ldoubletype;
		}
	}
}

Type *
mktype(Type *tp, uint8_t op,
       Symbol *sym, uint16_t nelem)
{
	static Type *typetab[NR_TYPE_HASH], **tbl;
	static uint8_t t;
	register Type *bp;
	char letter;

	if (op == PTR && tp == voidtype)
		return pvoidtype;
	t = (op  ^  (uint8_t) ((unsigned short) tp >> 3))
	         & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	if (op != FTN || op != STRUCT || op != UNION || op != ENUM) {
		for (bp = *tbl; bp; bp = bp->next) {
			if (bp->type == tp && bp->op == op &&
			    bp->sym == sym && bp->u.nelem == nelem) {
				return bp;
			}
		}
	}

	switch (op) {
	case PTR:    letter = 'R'; break;
	case FTN:    letter = 'F'; break;
	case ARY:    letter = 'V'; break;
	case ENUM:   letter = 'E'; break;
	case STRUCT: letter = 'S'; break;
	default: letter = tp->letter;
	}
	bp = xmalloc(sizeof(*bp));
	bp->next = *tbl;
	bp->type = tp;
	bp->op = op;
	bp->u.nelem = nelem;
	bp->sym = sym;
	bp->letter = letter;
	return *tbl = bp;
}

Type *
qualifier(Type *tp, uint8_t qlf)
{
	uint8_t q = tp->op;

	if (!qlf)
		return tp;
	if (q & TQUALIFIER) {
		if (q == qlf)
			return tp;
		tp = tp->type;
		qlf |= q;
	}
	return mktype(tp, qlf|TQUALIFIER, NULL, 0);
}

