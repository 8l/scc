
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

Node regs[] = {
	[E] =  {
		.op = REG,
		.reg = E
	},
	[D] = {
		.op = REG,
		.reg = D
	},
	[H] = {
		.op = REG,
		.reg = H
	},
	[L] = {
		.op = REG,
		.reg = L
	},
	[C] = {
		.op= REG,
		.reg = C
	},
	[B] = {
		.op= REG,
		.reg = B
	},
	[A] = {
		.op= REG,
		.reg = A
	},
	[IYL] = {
		.op = REG,
		.reg = IYL
	},
	[IYH] = {
		.op = REG,
		.reg = IYH
	},
	[DE] = {
		.op = REG,
		.reg = DE
	},
	[HL] = {
		.op = REG,
		.reg = HL
	},
	[BC] = {
		.op = REG,
		.reg = BC
	},
	[IX] = {
		.op = REG,
		.reg = IX
	},
	[IY] = {
		.op = REG,
		.reg = IY
	},
	[SP] = {
		.op = REG,
		.reg = SP
	}
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

static void moveto(Node *np, uint8_t reg);

static void
spill(uint8_t reg)
{
	Node *np, *r;
	Symbol *sym;
	uint8_t p, h, l, new;

	if ((np = reguse[reg]) == NULL)
		return;
	sym = np->sym;
	r = &regs[reg];

	switch (np->type.size) {
	case 1:
		if (sym) {
			code(LDL, np, r);
			np->op = sym->kind;
			sym->dirty = 0;
		} else {
			new = allocreg(np);
			moveto(np, new);
		}
		break;
	default:
		abort();
	}

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
	Node *r = &regs[reg], *u = reguse[reg];
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
		case MEM:
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
			code(LDL, &regs[lower[reg]], np);
			code(LDH, &regs[upper[reg]], np);
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
accum(Node *np)
{
	Symbol *sym;

	switch (np->type.size) {
	case 1:
		moveto(np, A);
		break;
	case 2:
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
	Symbol *sym;

	if (u && u->sym) {
		if (u->op == INDEX && np->sym == u->sym) {
			np->op = INDEX;
			return;
		} else {
			spill(HL);
		}
	}
	code(LDI, &regs[HL], np);
	if (sym = np->sym)
		sym->index = 1;
	np->op = INDEX;
	reguse[HL] = reguse[H] = reguse[L] = np;
}

static void
move(Node *np, Node *parent)
{
	assert(np->type.size == 1);
	switch (parent->op) {
	case OADD:
	case OSUB:
		switch (np->op) {
		case PAR:
		case AUTO:
		case CONST:
		case INDEX:
		case REG:
			return;
		case MEM:
			index(np);
			break;
		default:
			abort();
		}
		break;
	default:
		abort();
	}
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
	Node *lp = np->left, *rp = np->right, *a;

	switch (np->type.size) {
	case 1:
		a = reguse[A];
		if (a == lp)
			goto add1;
		if (a == rp)
			goto conmute1;
		if (lp->op == CONST) {
			accum(rp);
			goto conmute1;
		}
		accum(lp);
		goto add1;
	conmute1:
		conmute(np);
		lp = np->left, rp = np->right;
	add1:
		move(rp, np);
		code((np->op == OADD) ? ADD : SUB, lp, rp);
		np->op = REG;
		np->reg = A;
		reguse[A] = np;
		break;
	default:
		abort();
	}
}

static void
assign(Node *np)
{
	Node *lp = np->left, *rp = np->right;
	Symbol *sym = lp->sym;

	assert(rp->op == REG);
	/* TODO: what happens with the previous register? */
	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case MEM:
			if (sym && sym->index)
				lp->op = INDEX;
		case INDEX:
		case AUTO:
			code(LDL, lp, rp);
			break;
		case REG:
			code(MOV, lp, rp);
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

static void
ret(Node *np)
{
}

static void (*opnodes[])(Node *) = {
	[OADD] = add,
	[OSUB] = add,
	[OASSIG] = assign,
	[ORET] = ret
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
	uint8_t size = curfun->u.f.locals;
	char frame =  size != 0 || odebug;

	if (frame) {
		code(PUSH, NULL, &regs[IX]);
		code(MOV, &regs[IX], &regs[SP]);
		if (size > 6) {
			code(MOV, &regs[HL], imm(-size));
			code(ADD, &regs[HL], &regs[SP]);
			code(MOV, &regs[SP], &regs[HL]);
		} else {
			for (; size != 0; size-= 2)
				code(PUSH, NULL, &regs[HL]);
		}
	}

	apply(applycgen);

	if (frame) {
		code(MOV, &regs[SP], &regs[IX]);
		code(POP, &regs[IX], NULL);
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
