
#include <assert.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"


static Node *reguse[NREGS];
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
	static char reg16[] = {BC, DE, IY, 0};
	char *bp, c;

	switch (np->type.size) {
	case 1:
		for (bp = reg8; (c = *bp); ++bp) {
			if (reguse[c])
				continue;
			reguse[c] = np;
			return c;
		}
		/* TODO: Move variable to stack using PUSH/POP */
		break;
	case 2:
		for (bp = reg16; (c = *bp); ++bp) {
			char u = upper[c], l = lower[c];

			if (reguse[u] || reguse[l])
				continue;
			reguse[u] = reguse[l] = np;
			return c;
		}
		/* TODO: Move variable to stack using PUSH/POP */
		break;
	}
	abort();
}

static void
moveto(Node *np, uint8_t reg)
{
	Symbol *sym;

	switch (np->op) {
	case CONST:
		switch (np->type.size) {
		case 1:
		case 2:
			code(LDI, regs[reg], np);
			break;
		default:
			abort();
		}
		break;
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
	np->op = REG;
	sym->reg = np->u.reg = reg;
}

static void
move(Node *np)
{
	moveto(np, allocreg(np));
}

static void
push(Node *np)
{
	code(PUSH, NULL, np);
	np->op = PUSHED;
}

static void
accum(Node *np)
{
	switch (np->type.size) {
	case 1:
		if (reguse[A])
			push(reguse[A]);
		moveto(np, A);
		break;
	case 2:
		if (reguse[H] || reguse[L])
			push(&reg_HL);
		moveto(np, HL);
		break;
	case 4:
	case 8:
		abort();
	}
}

static void
index(Node *np)
{
	if (reguse[H] || reguse[L])
		push(&reg_HL);
	code(MOV, &reg_HL, np);
	np->op = INDEX;
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
add(Node *np)
{
	Node *lp = np->left, *rp = np->right;

	if (rp->op == REG || lp->op == CONST) {
		conmute(np);
		lp = np->left;
		rp = np->right;
	}
	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case PAR:
		case AUTO:
			switch (rp->op) {
			case MEM:
				index(rp);
			case PAR:
			case AUTO:
			case CONST:
				accum(lp);
				break;
			default:
				abort();
			}
			break;
		case MEM:
			if (reguse[A]) {
				switch (rp->op) {
				case PAR:
				case AUTO:
				case CONST:
					break;
				case MEM:
					index(rp);
					goto add_A;
				default:
					abort();
				}
			}
			accum(lp);
			break;
		case REG:
			if (lp->u.reg != A)
				moveto(lp, A);
			switch (rp->op) {
			case REG:
			case AUTO:
			case PAR:
			case CONST:
			case MEM:
				break;
			default:
				abort();
			}		
			break;			
		default:
			abort();
		}
	add_A:
		code(ADD, lp, rp);
		np->op = REG;
		np->u.reg = A;
		break;
	case 2:
	case 4:
	case 8:
		abort();
	}
}

static void
assign(Node *np)
{
	Node *lp = np->left, *rp = np->right;

	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case AUTO:
			code(LDL, lp, rp);
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
}


static void (*opnodes[])(Node *) = {
	[OADD] = add,
	[OASSIG] = assign
};

static void
cgen(Node *np, Node *parent)
{
	Node *lp, *rp;

	if (!np)
		return;

	if (np->addable >= ADDABLE)
		return;

	lp = np->left;
	rp = np->right;
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

	(*opnodes[np->op])(np);
}

static Node *
applycgen(Node *np)
{
	cgen(np, NULL);
	return NULL;
}

void
generate(void)
{
	extern char odebug;
	char frame = curfun->u.f.locals != 0 || odebug;

	if (frame) {
		code(PUSH, NULL, regs[IX]);
		code(MOV, regs[IX], regs[SP]);
		code(MOV, regs[HL], imm(-curfun->u.f.locals, &l_int16));
		code(ADD, regs[HL], regs[SP]);
		code(MOV, regs[SP], regs[HL]);
	}

	apply(applycgen);

	if (frame) {
		code(MOV, regs[SP], regs[IX]);
		code(POP, regs[IX], NULL);
		code(RET, NULL, NULL);
	}
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps (/sys/doc/compiler.ps)
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

void
addable(void)
{
	apply(genaddable);
}
