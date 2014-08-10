
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <cc.h>
#include "cc2.h"

#include <stdio.h>

void
genstack(Symbol *fun)
{
	Symbol *p;
	short off;

	for (off = 0, p = fun->u.f.vars; p; p = p->next) {
		if (p->u.v.sclass == AUTO) {
			p->u.v.off = off;
			off += p->u.v.type->align;
		}
	}

	fun->u.f.stack = off;
}

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
xcgen(Node *np)
{
	Node *lp, *rp;
	/* TODO: define a macro with the default integer type */
	unsigned imm, off;

	if (!np || np->complex == 0)
		return;
	lp = np->left;
	rp = np->right;
	if (!lp) {
		xcgen(rp);
	} else if (!rp) {
		xcgen(lp);
	} else {
		Node *p, *q;
		if (lp->complex > rp->complex)
			p = lp, q = rp;
		else
			p = rp, q = lp;
		xcgen(p);
		xcgen(q);
	}

	switch (np->op) {
	case OINC:
	case OADD: case OASSIG: case OMOD:
		break;
	default:
		abort();
	}
}

void
cgen(Symbol *sym, Node *list[])
{
	extern char odebug;
	Node *np;
	char frame = sym->u.f.stack != 0 || odebug;

	emit(ADDR, sym->u.f.name);
	if (frame) {
		emit(PUSH, IX);
		emit(LD, IX, SP);
		emit(LDI, HL, -sym->u.f.stack);
		emit(ADD, HL, SP);
		emit(LD, SP, HL);
	}

	while (np = *list++)
		xcgen(np);

	if (frame) {
		emit(LD, SP, IX);
		emit(POP, IX);
		emit(RET);
	}
}

/*
 * calculate addresability as follows
 *     AUTO => 11
 *     REGISTER => 13
 *     STATIC => 12
 *     CONST => 20
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
	case OINC:
	case OASSIG: case OADD: case OSUB: case OMOD:
		xaddable(lp);
		xaddable(rp);
		break;
	default:
		abort();
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
