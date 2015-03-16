
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
	[MOV] = inst2,
	[ADD] = inst2,
	[PUSH] = inst1,
	[POP] = inst1,
	[RET] = inst0,
	[NOP] = inst0,
	[INC] = inst1
};

static char *insttext[] = {
	[LDW] = "LD",
	[LDL] = "LD",
	[LDH] = "LD",
	[MOV] = "LD",
	[ADD] = "ADD",
	[PUSH] = "PUSH",
	[POP] = "POP",
	[RET] = "RET",
	[NOP] = "NOP",
	[INC] = "INC"
};

typedef struct inst Inst;
typedef struct addr Addr;

struct addr {
	char kind;
	union {
		uint8_t reg;
		TINT i;
		Inst *pc;
		Symbol *sym;
	} u;
};

struct inst {
	char op;
	Addr from, to;
	Inst *next;
};
Inst *prog, *pc;

Inst *
nextpc(void)
{
	Inst *new;

	new = xmalloc(sizeof(*new));
	if (!pc)
		prog = new;
	else
		pc->next = new;
	pc = new;
	pc->op = NOP;
	pc->to.kind = NONE;
	pc->from.kind = NONE;
	pc->next = NULL;
	return pc;
}

void
addr(char op, Node *np, Addr *addr)
{
	switch (addr->kind = np->op) {
	case REG:
		addr->u.reg = np->u.reg;
		break;
	case CONST:
		addr->u.i = np->u.imm;
		break;
	case AUTO:
		addr->u.i = np->u.sym->u.v.off;
		break;
	case MEM:
		addr->u.sym = np->u.sym;
		break;
	default:
		abort();
	}
}

void
code(uint8_t op, Node *to, Node *from)
{
	Inst *ip;

	ip = nextpc();
	if (from)
		addr(op, from, &ip->from);
	if (to)
		addr(op, to, &ip->to);
	ip->op = op;
}

void
writeout(void)
{
	if (!prog)
		return;

	for (pc = prog; pc; pc = pc->next)
		(*instcode[pc->op])();
}

static void
addr2txt(Addr *a)
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
		printf("(IX+%d)", a->u.i);
		break;
	case INDEX:
		fputs("(HL)", stdout);
		break;
	case MEM:
		sym = a->u.sym;
		if (sym->name)
			printf("(%s)", sym);
		else
			printf("(T%u)", sym->id);
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
	printf("\t%s\t", insttext[pc->op]);
	addr2txt((pc->to.kind != NONE) ? &pc->to : &pc->from);
	putchar('\n');
}

static void
inst2(void)
{
	printf("\t%s\t", insttext[pc->op]);
	addr2txt(&pc->to);
	putchar(',');
	addr2txt(&pc->from);
	putchar('\n');
}
