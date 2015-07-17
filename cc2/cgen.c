
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"

static Symbol retlabel = {
	.kind = LABEL
};

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

static Node regs[] = {
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

static void moveto(Node *np, uint8_t reg);

static void
allocreg(Node *np)
{
	static uint8_t reg8[] = {A, B, C, D, E, H, L, IYL, IY, 0};
	static uint8_t reg16[] = {BC, DE, IY, 0};
	Node *r;
	uint8_t *ary, *bp, c;

	switch (np->type.size) {
	case 1:
		ary = reg8;
		break;
	case 2:
		ary = reg16;
		break;
	default:
		abort();
	}
	for (bp = ary; c = *bp; ++bp) {
		r = reguse[c];
		if (!r || r->used) {
			moveto(np, c);
			return;
		}
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
	sym = np->sym;
	r = &regs[reg];

	switch (np->type.size) {
	case 1:
		if (sym) {
			code(LDL, np, r);
			np->op = sym->kind;
		} else {
			allocreg(np);
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
		if (sym && sym == u->sym)
			return;
		else if (!np->used)
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
	switch (np->type.size) {
	case 1:
		moveto(np, A);
		break;
	case 2:
		moveto(np, HL);
		break;
	default:
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
	case OASSIG:
		allocreg(np);
		break;
	case ONEG:
	case OCPL:
		switch (np->op) {
		case REG:
			if (np->reg == A)
				break;
			/* PASSTHROUGH */
		case PAR:
		case AUTO:
		case CONST:
		case MEM:
		case INDEX:
			accum(np);
			break;
		default:
			abort();
		}
		break;
	case OADD:
	case OSUB:
	case OBAND:
	case OBOR:
	case OBXOR:
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
	uint8_t op;

	switch (np->type.size) {
	case 1:
		a = reguse[A];
		if (a == lp)
			goto update_a;
		if (a == rp)
			goto conmute1;
		if (lp->op == CONST) {
			accum(rp);
			goto conmute1;
		}
		accum(lp);
		goto update_a;
	conmute1:
		conmute(np);
		lp = np->left, rp = np->right;
	update_a:
		move(rp, np);
		switch (np->op) {
		case OADD:
			op = ADD;
			break;
		case OSUB:
			op = SUB;
			break;
		case OBAND:
			op = AND;
			break;
		case OBOR:
			op = OR;
			break;
		case OBXOR:
			op = XOR;
			break;
		default:
			abort();
		}
		code(op, lp, rp);
		lp->used = rp->used = 1;
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

	switch (np->type.size) {
	case 1:
		switch (lp->op) {
		case MEM:
			if (sym && sym->index)
				lp->op = INDEX;
			/* PASSTROUGH */
		case INDEX:
		case AUTO:
			if (rp->op != REG)
				move(rp, np);
			lp->reg = rp->reg;
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

	np->op = REG;
	np->reg = lp->reg;
	np->sym = rp->sym = lp->sym;
	np->used = rp->used = lp->used = 1;
}

static void
ret(Node *np)
{
	static Node retnode = {
		.op = LABEL,
		.sym = &retlabel
	};

	if (np->left)
		accum(np->left);
	code(JP, &retnode, NULL);
}

static void
nop(Node *np)
{
}

static void
cpl(Node *np)
{

	Node *lp = np->left;
	uint8_t op;

	switch (np->type.size) {
	case 1:
		move(lp, np);
		switch (np->op) {
		case OCPL:
			op = CPL;
			break;
		case ONEG:
			op = NEG;
			break;
		default:
			abort();
		}
		code(op, lp, NULL);
		lp->used = 1;
		np->sym = lp->sym;
		np->reg = lp->reg;
		np->op = REG;
		reguse[A] = np;
		break;
	default:
		abort();
	}
}

static void
cast(Node *np)
{
	Node *lp = np->left;
	uint8_t reg;

	if (lp->type.size != np->type.size)
		abort();
	lp->used = 1;
	np->sym = lp->sym;
	if ((np->op = lp->op) == REG) {
		reg = lp->reg;
		np->reg = reg;
		reguse[pair[reg]] = reguse[reg] = np;
	}
}

static void (*opnodes[])(Node *) = {
	[OADD] = add,
	[OSUB] = add,
	[OASSIG] = assign,
	[ORET] = ret,
	[MEM] = nop,
	[REG] = nop,
	[AUTO] = nop,
	[CONST] = nop,
	[PAR] = nop,
	[OBOR] = add,
	[OBAND] = add,
	[OBXOR] = add,
	[OCPL] = cpl,
	[ONEG] = cpl,
	[OCAST] = cast
};

static void
cgen(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return;

	if (np->addable >= ADDABLE)
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
	(*opnodes[np->op])(np);
}

void
generate(void)
{
	uint8_t size = curfun->u.f.locals;
	static short id = 1000;
	Node **stmt, *np;

	retlabel.id = id++;

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

	for (stmt = curfun->u.f.body; np = *stmt; ++stmt)
		cgen(np);

	code(MOV, &regs[SP], &regs[IX]);
	retlabel.u.pc = pc;
	pc->label = &retlabel;
	code(POP, &regs[IX], NULL);
	code(RET, NULL, NULL);
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
