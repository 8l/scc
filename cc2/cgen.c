
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <cc.h>
#include "cc2.h"

#include <stdio.h>


enum {
	PUSH, POP, LD, ADD, RET, ADDI, LDI, ADDR, ADDX, ADCX, LDX
};

enum {
	AF, HL, DE, BC, IX, IY, SP, A, F, B, C, D, E, H, L, IXL, IXH, IYL, IYH
};

char *opnames[] = {
	[PUSH] = "PUSH", [POP] = "POP", [LD]  = "LD", [ADD] = "ADD",
	[RET]  = "RET" , [ADDI]= "ADD", [LDI] = "LD", [ADDX] = "ADD",
	[ADCX] = "ADC" , [LDX] = "LD"
};

char *regnames[] = {
	[AF] = "AF", [HL] = "HL", [DE] = "DE", [BC] = "BC", [IX] = "IX",
	[IY] = "IY", [SP] = "SP", [A]  = "A",  [F]  = "F",  [B]  = "B",
	[C]  = "C",  [D]  = "D",  [E]  = "E",  [H]  = "H",  [L]  = "L",
	[IXL]= "IXL",[IXH]= "IXH",[IYL]= "IYL",[IYH]= "IYH"
};

void
emit(char op, ...)
{
	va_list va;
	uint8_t reg1, reg2;
	/* TODO: define a macro with the default integer type */
	short imm, off;
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
		printf("\t%s\t%s,%hd\n", opnames[op], regnames[reg1], imm);
		break;
	case LDX:
		reg1 = va_arg(va, int);
		off = va_arg(va, int);
		reg2 = va_arg(va, int);
		printf("\t%s\t(%s%+hd),%s\n",
		       opnames[op], regnames[reg1], off, regnames[reg2]);
		break;
	case ADDX: case ADCX:
		reg1 = va_arg(va, int);
		reg2 = va_arg(va, int);
		off = va_arg(va, int);
		printf("\t%s\t%s,(%s%+hd)\n",
		       opnames[op], regnames[reg1], regnames[reg2], off);
		break;
	case ADDR:
		label = va_arg(va, char *);
		printf("%s:\n", label);
		break;
	}

	va_end(va);
}

void
cgen(Node *np)
{
	Node *lp, *rp;
	INT imm;

	if (!np || np->complex == 0)
		return;
	lp = np->left;
	rp = np->right;
	if (!lp) {
		cgen(rp);
	} else if (!rp) {
		cgen(lp);
	} else {
		Node *p, *q;
		if (lp->complex > rp->complex)
			p = lp, q = rp;
		else
			p = rp, q = lp;
		cgen(p);
		cgen(q);
	}

	switch (np->op) {
	default:
		abort();
	}
}

void
generate(Symbol *sym, Node *list[])
{
	extern char odebug;
	Node *np;
	char frame = sym->u.f.locals != 0 || odebug;

	emit(ADDR, sym->name);
	if (frame) {
		emit(PUSH, IX);
		emit(LD, IX, SP);
		emit(LDI, HL, -sym->u.f.locals);
		emit(ADD, HL, SP);
		emit(LD, SP, HL);
	}

	while (np = *list++)
		cgen(np);

	if (frame) {
		emit(LD, SP, IX);
		emit(POP, IX);
		emit(RET);
	}
}

/*
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REGISTER => 13      register
 *     STATIC => 12        (value)
 *     CONST => 20         $value
 */
static void
xaddable(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return;

	np->complex = 0;
	np->addable = 0;
	lp = np->left;
	rp = np->right;
	switch (np->op) {
	case AUTO:
		np->addable = 11;
		break;
	case REG:
		np->addable = 13;
		break;
	case MEM:
		np->addable = 12;
		break;
	case CONST:
		np->addable = 20;
		break;
	default:
		if (lp)
			xaddable(lp);
		if (rp)
			xaddable(rp);
		break;
	}

	if (np->addable > 10)
		return;
	if (lp)
		np->complex = lp->complex;
	if (rp) {
		int8_t d = np->complex - rp->complex;

		if (d == 0)
			++np->complex;
		else if (d < 0)
			np->complex = rp->complex;
	}
	if (np->complex == 0)
		++np->complex;
	return;
}

void
genaddable(Node *list[])
{
	Node *np;

	while (np = *list++)
		xaddable(np);
}

