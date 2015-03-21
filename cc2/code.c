
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "../inc/cc.h"
#include "cc2.h"

static char *regnames[] = {
	[AF] = "AF",
	[HL] = "HL", [DE] = "DE", [BC] = "BC",
	[IX] = "IX", [IY] = "IY", [SP] = "SP",
	[A]  = "A",
	[B]  = "B", [C]  = "C",
	[D]  = "D",  [E]  = "E",
	[H]  = "H",  [L]  = "L",
	[IYL]= "IYL",[IYH]= "IYH"
};

static void inst0(void), inst1(void), inst2(void);

static void (*instcode[])(void) = {
	[LDW] = inst2,
	[LDL] = inst2,
	[LDH] = inst2,
	[LDI] = inst2,
	[MOV] = inst2,
	[ADD] = inst2,
	[PUSH] = inst1,
	[POP] = inst1,
	[RET] = inst0,
	[NOP] = inst0,
	[INC] = inst1,
	[SUB] = inst2,
	[DEC] = inst1,
	[JP] = inst1,
	[AND] = inst2,
	[OR] = inst2,
	[XOR] = inst2,
	[CPL] = inst1,
	[NEG] = inst1

};

static char *insttext[] = {
	[LDW] = "LD",
	[LDL] = "LD",
	[LDH] = "LD",
	[LDI] = "LD",
	[MOV] = "LD",
	[ADD] = "ADD",
	[PUSH] = "PUSH",
	[POP] = "POP",
	[RET] = "RET",
	[NOP] = "NOP",
	[INC] = "INC",
	[SUB] = "SUB",
	[DEC] = "DEC",
	[JP] = "JP",
	[AND] = "AND",
	[OR] = "OR",
	[XOR] = "XOR",
	[CPL] = "CPL",
	[NEG] = "NEG"
};

Inst *pc, *prog;

static void
nextpc(void)
{
	Inst *new;

	new = xmalloc(sizeof(*new));

	if (!pc) {
		new->next = NULL;
		prog = new;
	} else {
		new->next = pc->next;
		pc->next = new;
	}

	new->prev = pc;
	new->to.kind = NONE;
	new->from.kind = NONE;
	pc = new;
}

void
addr(char op, Node *np, Addr *addr)
{
	switch (addr->kind = np->op) {
	case REG:
		addr->u.reg = np->reg;
		break;
	case CONST:
		addr->u.i = np->sym->u.imm;
		break;
	case AUTO:
		addr->u.i = np->sym->u.v.off;
		break;
	case LABEL:
	case MEM:
		addr->u.sym = np->sym;
		break;
	case INDEX:
		break;
	default:
		abort();
	}
}

void
code(uint8_t op, Node *to, Node *from)
{

	nextpc();
	if (from)
		addr(op, from, &pc->from);
	if (to)
		addr(op, to, &pc->to);
	pc->op = op;
}

void
inscode(uint8_t op, Addr *to, Addr *from)
{
	nextpc();
	if (from)
		pc->from = *from;
	if (to)
		pc->to = *to;
	pc->op = op;
}

void
delcode(void)
{
	Inst *prev = pc->prev, *next = pc->next;

	free(pc);
	if (!prev) {
		pc = next;
		prog = NULL;
	} else {
		pc = prev;
		prev->next = next;
		if (next)
			next->prev = prev;
	}
}

void
writeout(void)
{
	if (!prog)
		return;

	for (pc = prog; pc; pc = pc->next) {
		if (pc->label)
			printf("L%d:", pc->label->id);
		(*instcode[pc->op])();
	}
}

static void
addr2txt(uint8_t op, Addr *a)
{
	Symbol *sym;

	switch (a->kind) {
	case REG:
		fputs(regnames[a->u.reg], stdout);
		break;
	case CONST:
		printf("%d", a->u.i);
		break;
	case PAR:
	case AUTO:
		printf("(IX%+d)", a->u.i);
		break;
	case LABEL:
		sym = a->u.sym;
		printf("L%d", sym->id);
		break;
	case INDEX:
		fputs("(HL)", stdout);
		break;
	case MEM:
		sym = a->u.sym;
		if (sym->name)
			printf((op == LDI) ? "%s" : "(%s)", sym->name);
		else
			printf((op == LDI) ? "T%u" : "(T%u)", sym->id);
		break;
	default:
		abort();
	}
}

static void
inst0(void)
{
	printf("\t%s\n", insttext[pc->op]);
}

static void
inst1(void)
{
	uint8_t op = pc->op;

	printf("\t%s\t", insttext[op]);
	addr2txt(op, (pc->to.kind != NONE) ? &pc->to : &pc->from);
	putchar('\n');
}

static void
inst2(void)
{
	uint8_t op = pc->op;

	printf("\t%s\t", insttext[op]);
	addr2txt(op, &pc->to);
	putchar(',');
	addr2txt(op, &pc->from);
	putchar('\n');
}
