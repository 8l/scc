
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
	short siz;

	for (siz = 0, p = fun->u.f.vars; p; p = p->next)
		siz += p->u.v.type->size;
	fun->u.f.stack = siz;
}

enum {
	PUSH, POP, LD, ADD, RET, ADDI, LDI, ADDR
};

enum {
	AF, HL, DE, BC, IX, IY, SP, A, F, B, C, D, E, H, L, IXL, IXH, IYL, IYH
};

char *opnames[] = {
	[PUSH] = "PUSH", [POP] = "POP", [LD]  = "LD", [ADD] = "ADD",
	[RET]  = "RET" , [ADDI]= "ADD", [LDI] = "LD"
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
	short imm;
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
		printf("\t%s\t%s,%hX\n", opnames[op], regnames[reg1], imm);
		break;
	case ADDR:
		label = va_arg(va, char *);
		printf("%s:\n", label);
		break;
	}

	va_end(va);
}

void
cgen(Symbol *sym, Node *list[])
{
	extern char odebug;
	char frame = sym->u.f.stack != 0 || odebug;

	emit(ADDR, sym->u.f.name);
	if (frame) {
		emit(PUSH, IX);
		emit(LD, IX, SP);
		emit(LDI, HL, -sym->u.f.stack);
		emit(ADD, HL, SP);
		emit(LD, SP, HL);
	}


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
	case OADD:
	case OSUB:
		xaddable(lp);
		xaddable(rp);
	}
}

void
genaddable(Node *list[])
{
	Node *np;

	while ((np = *list++) != NULL)
		xaddable(np);
}
