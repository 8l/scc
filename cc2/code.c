
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cc.h>
#include "cc2.h"


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

void
emit(char op, ...)
{
	va_list va;
	uint8_t reg1, reg2;
	TINT imm;
	short off;
	char *label;

	va_start(va, op);
	switch (op) {
	case RET:
		printf("\t%s\n", opnames[op]);
		break;
	case PUSH: case POP:
		reg1 = va_arg(va, int);
		printf("\t%s\t%s\n", opnames[op], regnames[reg1]);
		break;
	case ADD: case LD:
		reg1 = va_arg(va, int);
		reg2 = va_arg(va, int);
		printf("\t%s\t%s,%s\n",
		       opnames[op], regnames[reg1], regnames[reg2]);
		break;
	case ADDI: case LDI:
		reg1 = va_arg(va, int);
		imm = va_arg(va, int);
		printf("\t%s\t%s,%d\n", opnames[op], regnames[reg1], imm);
		break;
	case ADDX: case ADCX: case LDFX:
		reg1 = va_arg(va, int);
		reg2 = va_arg(va, int);
		off = va_arg(va, int);
		printf("\t%s\t%s,(%s%+d)\n",
		       opnames[op], regnames[reg1], regnames[reg2], off);
		break;
	case LDX:
		reg1 = va_arg(va, int);
		off = va_arg(va, int);
		reg2 = va_arg(va, int);
		printf("\t%s\t(%s%+d),%s\n",
		       opnames[op], regnames[reg1], off, regnames[reg2]);
		break;
	case ADDR:
		label = va_arg(va, char *);
		printf("%s:\n", label);
		break;
	}

	va_end(va);
}
