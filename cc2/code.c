
#include <stdarg.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>

#include "../inc/cc.h"
#include "cc2.h"

typedef struct inst Inst;
typedef struct addr Addr;

struct addr {
	char kind;
	union {
		char reg;
		Inst *pc;
		Symbol *sym;
	} u;
};

struct inst {
	char op;
	Addr from, to;
	Inst *next;
};

static char *opnames[] = {
	[PUSH] = "PUSH", [POP] = "POP", [LD]  = "LD", [ADD] = "ADD",
	[RET]  = "RET" , [ADDI]= "ADD", [LDI] = "LD", [ADDX] = "ADD",
	[ADCX] = "ADC" , [LDX] = "LD" , [LDFX] = "LD"
};

static char *regnames[] = {
	[AF] = "AF", [HL] = "HL", [DE] = "DE", [BC] = "BC", [IX] = "IX",
	[IY] = "IY", [SP] = "SP", [A]  = "A",  [F]  = "F",  [B]  = "B",
	[C]  = "C",  [D]  = "D",  [E]  = "E",  [H]  = "H",  [L]  = "L",
	[IXL]= "IXL",[IXH]= "IXH",[IYL]= "IYL",[IYH]= "IYH", [I] = "I"
};

static char *opfmt[] = {
	[RET]  = "\to",
	[PUSH] = "\to\tr",
	[POP]  = "\to\tr",
	[ADD]  = "\to\tr,r",
	[LD]   = "\to\tr,r",
	[ADDI] = "\to\tr,i",
	[LDI]  = "\to\tr,i",
	[ADDX] = "\to\tr,(r+i)",
	[ADCX] = "\to\tr,(r+i)",
	[LDFX] = "\to\tr,(r+i)",
	[LDX]  = "\to\t(r+i),r",
};

Inst *prog, *pc;

Inst *
inst(uint8_t op)
{
	Inst *new;

	new = xmalloc(sizeof(*new));
	if (!pc)
		prog = new;
	else
		pc->next = new;
	pc = new;
	pc->op = op;
	pc->next = NULL;
	return pc;
}

void
code(char op, ...)
{
}
