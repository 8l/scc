
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"


static Node *reguse[NPAIRS];
static uint8_t upper[] = {[DE] = D, [HL] = H, [BC] = B,  [IY] = IYH};
static uint8_t lower[] = {[DE] = E, [HL] = L, [BC] = C, [IY] = IYL};
static uint8_t pair[] = {
	[A] = A,
	[H] = HL, [L] = HL, [HL] = HL,
	[B] = BC, [C] = BC, [BC] = BC,
	[D] = DE, [E] = DE, [DE] = DE,
	[IYL] = IY, [IYH] = IY, [IY] = IY
};

Node
reg_E = {
	.op = REG,
	.reg = E
},
reg_D = {
	.op = REG,
	.reg = D
},
reg_H = {
	.op = REG,
	.reg = H
},
reg_L = {
	.op = REG,
	.reg = L
},
reg_C = {
	.op= REG,
	.reg = C
},
reg_B = {
	.op= REG,
	.reg = B
},
reg_A = {
	.op= REG,
	.reg = A
},
reg_IYL = {
	.op = REG,
	.reg = IYL
},
reg_IYH = {
	.op = REG,
	.reg = IYH
},
reg_DE = {
	.op = REG,
	.reg = DE
},
reg_HL = {
	.op = REG,
	.reg = HL
},
reg_BC = {
	.op = REG,
	.reg = BC
},
reg_IX = {
	.op = REG,
	.reg = IX
},
reg_IY = {
	.op = REG,
	.reg = IY
},
reg_SP = {
	.op = REG,
	.reg = SP
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

static uint8_t
allocreg(Node *np)
{
	static uint8_t reg8[] = {A, B, C, D, E, H, L, IYL, IY, 0};
	static uint8_t reg16[] = {BC, DE, IY, 0};
	uint8_t *bp, c;

	switch (np->type.size) {
	case 1:
		bp = reg8;
		break;
	case 2:
		bp = reg16;
		break;
	default:
		abort();
	}
	while (c = *bp++) {
		if (reguse[c])
			continue;
		return c;
	}
	/* TODO: What to do here? */
	abort();
}

static void
spill(uint8_t reg)
{
	Node *np, *r;
	Symbol *sym;
	uint8_t p, h, l;

	if ((np = reguse[reg]) == NULL)
		return;
	if ((sym = np->sym) == NULL)
		goto freereg;
	if (!sym->dirty)
		goto freereg;

	r = regs[reg];
	switch (np->type.size) {
	case 1:
		code(LDL, np, r);
		break;
	default:
		abort();
	}
	sym->dirty = 0;

freereg:
	reguse[reg] = NULL;
	p = pair[reg];
	l = lower[p];
	h = upper[p];
	if (reg >= NREGS)
		reguse[l] = reguse[h] = NULL;
	else if (!reguse[l] && !reguse[h])
		reguse[p] = NULL;
}

static void
moveto(Node *np, uint8_t reg)
{
	Node *r = regs[reg], *u = reguse[reg];
	char op = np->op;

	if (u) {
		Symbol *sym = np->sym;
		if (op == u->op && sym && sym == u->sym)
			return;
		else
			spill(reg);
	}

	switch (np->type.size) {
	case 1:
		switch (op) {
		case AUTO:
			code(LDL, r, np);
			break;
		case CONST:
		case INDEX:
			code(LDI, r, np);
			break;
		case REG:
			code(MOV, r, np);
			break;
		default:
			abort();
		}
		break;
	case 2:
		switch (op) {
		case CONST:
			code(LDL, r, np);
			break;
		case AUTO:
			code(LDL, regs[lower[reg]], np);
			code(LDH, regs[upper[reg]], np);
			break;
		default:
			abort();
		}
		reguse[upper[reg]] = reguse[lower[reg]] =  np;
		break;
	default:
		abort();
	}
	reguse[pair[reg]] = reguse[reg] = np;
	np->op = REG;
	np->reg = reg;
}

static void
move(Node *np)
{
	moveto(np, allocreg(np));
}

static void
accum(Node *np)
{
	Symbol *sym;

	switch (np->type.size) {
	case 1:
		spill(A);
		moveto(np, A);
		break;
	case 2:
		spill(HL);
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
	Node *u = reguse[HL];

	if (u && u->sym) {
		if (u->op == INDEX && np->sym == u->sym) {
			np->op = INDEX;
			return;
		} else {
			spill(HL);
		}
	}
	code(LDI, &reg_HL, np);
	np->op = INDEX;
	reguse[HL] = reguse[H] = reguse[L] = np;
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
add(Node *np, uint8_t op)
{
	Node *lp = np->left, *rp = np->right;
	uint8_t i;

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
			if (lp->reg != A)
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
		code((op == OADD) ? ADD : SUB, lp, rp);
		np->op = REG;
		np->reg = A;
		break;
	case 2:
	case 4:
	case 8:
		abort();
	}
}

static void
assign(Node *np, uint8_t op)
{
	Node *lp = np->left, *rp = np->right;
	Symbol *sym = lp->sym;

	assert(rp->op == REG);
	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case AUTO:
			code(LDL, lp, rp);
			break;
		case REG:
			/* TODO: what happens with the previous register? */
			code(MOV, lp, rp);
			break;
		case MEM:
			/* TODO: check if the variable is indexed */
			index(lp);
			code(LDL, lp, rp);
			break;
		default:
			abort();
		}
		break;
	default:
		abort();
	}

	if (sym)
		sym->dirty = 0;
	np->op = REG;
	np->reg = rp->reg;
}

static void (*opnodes[])(Node *, uint8_t) = {
	[OADD] = add,
	[OSUB] = add,
	[OASSIG] = assign
};

static void
cgen(Node *np, Node *parent)
{
	Node *lp, *rp;
	uint8_t op;

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
	op = np->op;
	(*opnodes[op])(np, op);
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
	uint8_t i, size = curfun->u.f.locals;
	char frame =  size != 0 || odebug;

	if (frame) {
		code(PUSH, NULL, &reg_IX);
		code(MOV, &reg_IX, &reg_SP);
		if (size > 6) {
			code(MOV, &reg_HL, imm(-size));
			code(ADD, &reg_HL, &reg_SP);
			code(MOV, &reg_SP, &reg_HL);
		} else {
			for (i = size; i != 0; i-= 2)
				code(PUSH, NULL, &reg_HL);
		}
	}

	apply(applycgen);

	if (frame) {
		code(MOV, &reg_SP, &reg_IX);
		code(POP, &reg_IX, NULL);
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
