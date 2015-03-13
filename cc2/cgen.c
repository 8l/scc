
#include <assert.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"


static bool reguse[NREGS];
static char upper[] = {[DE] = D, [HL] = H, [BC] = B,  [IY] = IYH};
static char lower[] = {[DE] = E, [HL] = L, [BC] = C, [IY] = IYL};

Node
reg_E = {
	.op = REG,
	.u.reg = E
},
reg_D = {
	.op = REG,
	.u.reg = D
},
reg_H = {
	.op = REG,
	.u.reg = H
},
reg_L = {
	.op = REG,
	.u.reg = L
},
reg_C = {
	.op= REG,
	.u.reg = C
},
reg_B = {
	.op= REG,
	.u.reg = B
},
reg_A = {
	.op= REG,
	.u.reg = A
},
reg_IYL = {
	.op = REG,
	.u.reg = IYL
},
reg_IYH = {
	.op = REG,
	.u.reg = IYH
},
reg_DE = {
	.op = REG,
	.u.reg = DE
},
reg_HL = {
	.op = REG,
	.u.reg = HL
},
reg_BC = {
	.op = REG,
	.u.reg = BC
},
reg_IX = {
	.op = REG,
	.u.reg = IX
},
reg_IY = {
	.op = REG,
	.u.reg = IY
},
reg_SP = {
	.op = REG,
	.u.reg = SP
};

Node *regs[] = {
	[A] = &reg_A,
	[B] = &reg_B, [C] = &reg_C,
	[D] = &reg_D, [E] = &reg_E,
	[H] = &reg_H, [L] = &reg_L,
	[IYL] = &reg_IYL, [IYH] = &reg_IYH,
	[HL] = &reg_HL, [DE] = &reg_DE, [BC]= &reg_BC,
	[IX] = &reg_IX, [IY] = &reg_IY, [SP] = &reg_SP
};

static char
allocreg(Node *np)
{
	static char reg8[] = {A, B, C, D, E, H, L, IYL, IY, 0};
	static char reg16[] = {BC, HL, DE, IY, 0};
	char *bp, c;

	switch (np->type.size) {
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
	Symbol *sym;
	char reg;

	reg = allocreg(np);

	switch (np->op) {
	case AUTO:
		sym = np->u.sym;
		switch (np->type.size) {
		case 1:
			code(LDL, regs[reg], np);
			break;
		case 2:
			code(LDL, regs[lower[reg]], np);
			code(LDH, regs[upper[reg]], np);
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
cgen(Node *np, Node *parent)
{
	Node *lp, *rp;
	TINT imm;

	if (!np)
		return;

	lp = np->left;
	rp = np->right;
	if (np->addable >= ADDABLE) {
		if (parent && parent->op == OASSIG)
			return;
		move(np);
		return;
	}

	if (!lp) {
		cgen(rp, np);
	} else if (!rp) {
		cgen(lp, np);
	} else {
		Node *p, *q;
		if (lp->complex > rp->complex)
			p = lp, q = rp;
		else
			p = rp, q = lp;
		cgen(p, np);
		cgen(q, np);
	}

	switch (np->op) {
	case OADD:
		switch (np->type.size) {
		case 1:
			if (rp->u.reg == A) {
				conmute(np);
				lp = np->left;
				rp = np->right;
			} else if (lp->u.reg != A) {
				/* TODO: Move A to variable */
				code(PUSH, NULL, lp);
				code(LDL, regs[A], lp);
			}
			code(ADD, regs[A], rp);
			np->op = REG;
			np->u.reg = A;
			break;
		case 2:
			if (rp->u.reg == HL || rp->u.reg == IY) {
				conmute(np);
				lp = np->left;
				rp = np->right;
			} else if (lp->u.reg != HL && lp->u.reg != IY) {
				/* TODO: Move HL to variable */
				code(PUSH, NULL, lp);
				code(LDL, regs[L], lp);
				code(LDH, regs[H], lp);
			}
			code(ADD, lp, rp);
			np->op = REG;
			np->u.reg = lp->u.reg;
			break;
		case 4:
		case 8:
			abort();
		}
		break;
	case OASSIG:
		switch (np->type.size) {
		case 1:
			switch (lp->op) {
			case AUTO:
				code(LDL, rp, lp);
				break;
			case REG:
			case MEM:
			default:
				abort();
			}
			break;
		default:
			abort();
		}
		break;
	default:
		abort();
	}
}

static Node *
applycgen(Node *np)
{
	cgen(np, NULL);
	return NULL;
}

void
generate(Symbol *fun)
{
	extern char odebug;
	char frame = fun->u.f.locals != 0 || odebug;

	if (frame) {
		code(PUSH, NULL, regs[IX]);
		code(MOV, regs[IX], regs[SP]);
		code(MOV, regs[HL], imm(-fun->u.f.locals, &l_int16));
		code(ADD, regs[HL], regs[SP]);
		code(MOV, regs[SP], regs[HL]);
	}

	apply(fun->u.f.body, applycgen);

	if (frame) {
		code(MOV, regs[SP], regs[IX]);
		code(POP, regs[IX], NULL);
		code(RET, NULL, NULL);
	}
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REGISTER => 13      register
 *     STATIC => 12        (value)
 *     CONST => 20         $value
 */
Node *
genaddable(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return np;

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
			genaddable(lp);
		if (rp)
			genaddable(rp);
		break;
	}

	if (np->addable > 10)
		return np;
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
	return np;
}
