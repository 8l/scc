
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <cc.h>
#include "cc2.h"

#include <stdio.h>


enum {
	PUSH, POP, LD, ADD, RET, ADDI, LDI, ADDR, ADDX, ADCX, LDX,
	LDFX
};

enum {
	A, B, C, D, E, H, L, IYL, IYH, NREGS, IXL, IXH, F, I, SP, AF, HL, DE, BC, IX, IY
};

char *opnames[] = {
	[PUSH] = "PUSH", [POP] = "POP", [LD]  = "LD", [ADD] = "ADD",
	[RET]  = "RET" , [ADDI]= "ADD", [LDI] = "LD", [ADDX] = "ADD",
	[ADCX] = "ADC" , [LDX] = "LD" , [LDFX] = "LD"
};

char *regnames[] = {
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
	case LDFX:
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
	case ADDX: case ADCX:
		reg1 = va_arg(va, int);
		reg2 = va_arg(va, int);
		off = va_arg(va, int);
		printf("\t%s\t%s,(%s%+d)\n",
		       opnames[op], regnames[reg1], regnames[reg2], off);
		break;
	case ADDR:
		label = va_arg(va, char *);
		printf("%s:\n", label);
		break;
	}

	va_end(va);
}

static char
upper(char pair)
{
	switch (pair) {
	case HL: return H;
	case BC: return B;
	case DE: return D;
	case IY: return IYH;
	}
}

static char
lower(char pair)
{
	switch (pair) {
	case HL: return L;
	case DE: return E;
	case BC: return B;
	case IY: return IYL;
	}
}

static char
allocreg(Node *np)
{
	static bool regs[NREGS], *bp;

	switch (np->type->size) {
	case 1:
		for (bp = regs; bp < &regs[NREGS]; ++bp) {
			if (*bp)
				continue;
			*bp = 1;
			return bp - regs;
		}
		/* TODO: Move variable to stack using PUSH/POP */
		break;
	case 2:
		if (!regs[H] && !regs[L]) {
			regs[H] = regs [L] = 1;
			return HL;
		}
		if (!regs[D] && !regs[E]) {
			regs[D] = regs [E] = 1;
			return DE;
		}
		if (!regs[C] && !regs[B]) {
			regs[B] = regs [C] = 1;
			return BC;
		}
		if (!regs[IYL] && !regs[IYH]) {
			regs[IYL] = regs [IYH] = 1;
			return IY;
		}
		/* TODO: Move variable to stack using PUSH/POP */
	}
	abort();
}

static void
move(Node *np)
{
	Type *tp = np->type;
	Symbol *sym;
	char reg;

	switch (np->op) {
	case AUTO:
		sym = np->u.sym;
		switch (tp->size) {
		case 1:
			emit(LDFX, allocreg(np), sym->u.v.off);
			break;
		case 2:
			reg = allocreg(np);
			emit(LDFX, lower(reg), IX, sym->u.v.off);
			emit(LDFX, upper(reg), IX, sym->u.v.off+1);
			break;
		case 4:
		case 8:
		default:
			abort();
		}
		break;
	default:
		abort();
	}
}

static void
cgen(Node *np)
{
	Node *lp, *rp;
	TINT imm;

	if (!np)
		return;

	lp = np->left;
	rp = np->right;
	if (np->addable >= ADDABLE) {
		move(np);
		return;
	}

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

