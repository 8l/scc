
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "../inc/cc.h"
#include "cc2.h"

#include <stdio.h>

static bool reguse[NREGS];
static char upper[] = {[DE] = D, [HL] = H, [BC] = B, [IX] = IXH, [IY] = IYH};
static char lower[] = {[DE] = E, [HL] = L, [BC] = C, [IX] = IXL, [IY] = IYL};


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
			code(LDFX, reg, IX, sym->u.v.off);
			break;
		case 2:
			code(LDFX, lower[reg], IX, sym->u.v.off);
			code(LDFX, upper[reg], IX, sym->u.v.off+1);
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
		switch (np->type->size) {
		case 1:
			if (rp->u.reg == A) {
				conmute(np);
				lp = np->left;
				rp = np->right;
			} else if (lp->u.reg != A) {
				/* TODO: Move A to variable */
				code(PUSH, AF);
				code(LD, A, lp->u.reg);
			}
			code(ADD, A, rp->u.reg);
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
				code(PUSH, HL);
				code(LD, H, upper[lp->u.reg]);
				code(LD, L, lower[lp->u.reg]);
			}
			code(ADD, lp->u.reg, rp->u.reg);
			np->op = REG;
			np->u.reg = lp->u.reg;
			break;
		case 4:
		case 8:
			abort();
		}
		break;
	case OASSIG:
		switch (np->type->size) {
		case 1:
			switch (lp->op) {
			case AUTO:
				code(LDX, IX, lp->u.sym->u.v.off, rp->u.reg);
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
		code(PUSH, IX);
		code(LD, IX, SP);
		code(LDI, HL, -fun->u.f.locals);
		code(ADD, HL, SP);
		code(LD, SP, HL);
	}

	apply(fun->u.f.body, applycgen);

	if (frame) {
		code(LD, SP, IX);
		code(POP, IX);
		code(RET);
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
