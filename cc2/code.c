
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../inc/cc.h"
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

void
code(char op, ...)
{
	va_list va;
	char *cp, c;

	va_start(va, op);
	for (cp = opfmt[op]; c = *cp; ++cp) {
		switch (c) {
		case 'o':
			fputs(opnames[op], stdout);
			break;
		case 'r':
			fputs(regnames[va_arg(va, int)], stdout);
			break;
		case 'i':
			printf("%d", va_arg(va, int));
			break;
		default:
			putchar(c);
			break;
		}
	}
	putchar('\n');

	va_end(va);
}
