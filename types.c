
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sizes.h"
#include "cc.h"
#include "machine.h"

#define NR_TYPE_HASH 16

Type
	*voidtype = &(Type) {
		.op = VOID,
		.letter = 'W'
	},
	*booltype = &(Type) {
		.op = INT,
		.letter = 'B'
	},
	*uchartype = &(Type) {
		.op = INT,
		.letter = 'M',
		.sign = 1
	},
	*chartype = &(Type) {
		.op = INT,
		.letter = 'C',
	},
	*uinttype = &(Type) {
		.op = INT,
		.letter = 'U',
		.sign = 1
	},
	*inttype = &(Type) {
		.op = INT,
		.letter = 'I',
	},
	*ushortype = &(Type) {
		.op = INT,
		.letter = 'E'
	},
	*shortype = &(Type) {
		.op = INT,
		.letter = 'K',
	},
	*longtype = &(Type) {
		.op = INT,
		.letter = 'L'
	},
	*ulongtype = &(Type) {
		.op = INT,
		.letter = 'Z',
		.sign = 1
	},
	*ullongtype = &(Type) {
		.op = INT,
		.letter = 'O',
		.sign = 1
	},
	*llongtype = &(Type) {
		.op = INT,
		.letter = 'G',
	},
	*floattype = &(Type) {
		.op = FLOAT,
		.letter = 'F'
	},
	*doubletype = &(Type) {
		.op = FLOAT,
		.letter = 'D'
	},
	*ldoubletype = &(Type) {
		.op = FLOAT,
		.letter = 'H'
	};

Type *
ctype(int8_t type, int8_t sign, int8_t size)
{
	if (type == CHAR && !sign)
		sign = options.charsign;
	if (sign == SIGNED)
		sign = 0;
	if (type == DOUBLE)
		type = FLOAT, size += LONG;

	switch (type) {
	case VOID:                      return voidtype;
	case BOOL:                      return booltype;
	case CHAR:                      return (sign) ? uchartype  : chartype;
	case INT: switch (size) {
		case 0:                 return (sign) ? uinttype   : inttype;
		case SHORT:             return (sign) ? ushortype  : shortype;
		case LONG:              return (sign) ? longtype   : ulongtype;
		case LONG+LONG:         return (sign) ? ullongtype : llongtype;
		}
	case FLOAT: switch (size) {
		case 0:                 return floattype;
		case LONG:              return doubletype;
		case LONG+LONG:         return ldoubletype;
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

	t = (op  ^  (uint8_t) ((unsigned short) tp >> 3))
	         & NR_TYPE_HASH-1;
	tbl = &typetab[t];
	if (op != FTN || op != STRUCT || op != UNION || op != ENUM) {
		for (bp = *tbl; bp; bp = bp->next) {
			if (bp->type == tp && bp->op == op &&
			    bp->sym == sym && bp->nelem == nelem) {
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
	default:     abort();
	}
	bp = xmalloc(sizeof(*bp));
	bp->next = *tbl;
	bp->type = tp;
	bp->op = op;
	bp->nelem = nelem;
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

