
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <cc.h>
#include "cc2.h"

#include <stdio.h>

/* TODO: Change emit to code */
enum {
	PUSH, POP, LD, ADD, RET, ADDI, LDI, ADDR, ADDX, ADCX, LDX,
	LDFX
};

enum {
	A = 1, B, C, D, E, H, L, IYL, IYH, NREGS,
	IXL, IXH, F, I, SP, AF, HL, DE, BC, IX, IY
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

static bool reguse[NREGS];
static char upper[] = {[DE] = D, [HL] = H, [BC] = B, [IX] = IXH, [IY] = IYH};
static char lower[] = {[DE] = E, [HL] = L, [BC] = C, [IX] = IXL, [IY] = IYL};

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

static char
allocreg(Node *np)
{
	char reg8[] = {A, B, C, D, E, H, L, IYL, IY, 0};
	char reg16[] = {BC, HL, DE, IY, 0};
	char *bp, c;

	switch (np->type->size) {
	case 1:
		for (bp = reg8; (c = *bp); ++bp) {
			if (reguse[c])
				continue;
			reguse[c] = 1;
			return c;
		}
		/* TODO: Move variable to stack using PUSH/POP */
		break;
	case 2:
		for (bp = reg16; (c = *bp); ++bp) {
			char u = upper[c], l = lower[c];

			if (reguse[u] || reguse[l])
				continue;
			reguse[l] = 1;
			reguse[u];
			return c;
		}
		/* TODO: Move variable to stack using PUSH/POP */
		break;
	}
	abort();
}

static void
move(Node *np)
{
	Type *tp = np->type;
	Symbol *sym;
	char reg;

	reg = allocreg(np);

	switch (np->op) {
	case AUTO:
		sym = np->u.sym;
		switch (tp->size) {
		case 1:
			emit(LDFX, reg, IX, sym->u.v.off);
			break;
		case 2:
			emit(LDFX, lower[reg], IX, sym->u.v.off);
			emit(LDFX, upper[reg], IX, sym->u.v.off+1);
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
	/* TODO: Update symbol  if necessary */
	np->op = REG;
	np->u.reg = reg;
}

static void
conmute(Node *np)
{
	Node *p, *q;

	p = np->left;
	q = np->right;
	np->left = q;
	np->right = p;
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

	assert(lp && lp->op == REG || rp && rp->op == REG);
	switch (np->op) {
	case OADD:
		switch (np->type->size) {
		case 1:
			if (rp->u.reg == A) {
				conmute(np);
				lp = np->left;
				rp = np->right;
			} else if (lp->u.reg != A) {
				/* TODO: Move A to variable */
				emit(PUSH, AF);
				emit(LD, A, lp->u.reg);
			}
			emit(ADD, A, rp->u.reg);
			break;
		case 2:
			if (rp->u.reg == HL || rp->u.reg == IY) {
				conmute(np);
				lp = np->left;
				rp = np->right;
			} else if (lp->u.reg != HL && lp->u.reg != IY) {
				/* TODO: Move HL to variable */
				emit(PUSH, HL);
				emit(LD, H, upper[lp->u.reg]);
				emit(LD, L, lower[lp->u.reg]);
			}
			emit(ADD, lp->u.reg, rp->u.reg);
			break;
		case 4:
		case 8:
			abort();
		}
		break;
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

