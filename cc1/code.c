
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/cc.h"
#include "cc1.h"

char *opcodes[] = {
	[OADD] = "+",
	[OSUB] = "-",
	[OMUL] = "*",
	[OINC] = ";+",
	[ODEC] =  ";-",
	[OSIZE] = "#",
	[OPTR] = "@",
	[OMOD] = "%",
	[ODIV] = "/",
	[OSHL] = "l",
	[OSHR]  = "r",
	[OLT] = "<",
	[OGT] = ">",
	[OGE] = "]",
	[OLE] =  "[",
	[OEQ] = "=",
	[ONE] = "!",
	[OBAND] = "&",
	[OBXOR]  = "^",
	[OBOR] = "|",
	[OASSIGN] = ":",
	[OA_MUL] = ":*",
	[OA_DIV] = ":/",
	[OA_MOD] = ":%",
	[OA_ADD] = ":+",
	[OA_SUB] = ":-",
	[OA_SHL] = ":l",
	[OA_SHR] = ":r",
	[OA_AND] = ":&",
	[OA_XOR] = ":^",
	[OA_OR] = ":|",
	[OADDR] = "a",
	[ONEG] = "_",
	[OCPL] = "~",
	[OAND] = "y",
	[OOR] = "o",
	[OCOMMA] = ","
};

void
freetree(Node *np)
{
	Node **p;

	if (!np)
		return;

	for (p = np->childs; np->nchilds--; ++p)
		freetree(*p);
	free(np);
}

static void
emitvar(Symbol *sym)
{
	char c;

	if (sym->isstatic && !sym->isglobal)
		c = 'T';
	else if (sym->isstatic && sym->isglobal)
		c = 'Y';
	else if (sym->isglobal)
		c = 'G';
	else if (sym->isregister)
		c = 'R';
	else if (sym->isfield)
		c = 'M';
	else if (sym->isparameter)
		c = 'P';
	else
		c = 'A';
	printf("%c%d", c, sym->id);
}

static void
emitconst(Node *np)
{
	char *bp, c;
	Symbol *sym = np->u.sym;

	if (np->type == inttype) {
		printf("#%c%x", np->type->letter, sym->u.i);
	} else {
		putchar('"');
		for (bp = sym->u.s; c = *bp; ++bp)
			printf("%02x", (unsigned) c);
	}
}

void
emitstruct(Symbol *sym)
{
	printf("S%d\t(\n", sym->id);
}

void
emitestruct(void)
{
	puts(")");
}

void
emitsym(Node *np)
{
	putchar('\t');
	(np->b.constant) ? emitconst(np) : emitvar(np->u.sym);
}

static void
emittype(Type *tp)
{
	putchar(tp->letter);
}

void
emitdcl(Symbol *sym)
{
	emitvar(sym);
	putchar('\t');
	emittype(sym->type);
	putchar('\n');
}

void
emitcast(Node *np)
{
	Node *child = np->childs[0];

	(*child->code)(child);
	printf("\t%c%c", child->type->letter, np->type->letter);
}

void
emitunary(Node *np)
{
	Node *child;
	char letter;

	letter = np->type->letter;
	child = np->childs[0];
	(*child->code)(child);
	printf("\t%s%c", opcodes[np->u.op], letter);
}

void
emitbin(Node *np)
{
	Node *child1, *child2;

	child1 = np->childs[0];
	child2 = np->childs[1];
	(*child1->code)(child1);
	(*child2->code)(child2);
	printf("\t%s%c", opcodes[np->u.op], np->type->letter);
}

void
emitternary(Node *np)
{
	Node *cond, *ifyes, *ifno;

	cond = np->childs[0];
	ifyes = np->childs[1];
	ifno = np->childs[2];
	(*cond->code)(cond);
	(*ifyes->code)(ifyes);
	(*ifno->code)(ifno);
	printf("\t?%c", np->type->letter);
}

void
emitsizeof(Node *np)
{
	printf("\t#%c", np->u.type->letter);
}

void
emitexp(Node *np)
{
	if (np)
		(*np->code)(np);
	putchar('\n');
}

void
emitprint(Node *np)
{
	(*np->code)(np);
	printf("\tk%c\n", np->type->letter);
	fflush(stdout);
}

void
emitfun(Symbol *sym)
{
	printf("%c%d\tF\t%s\t{\n",
	       sym->isglobal ? 'G' : 'Y', sym->id, sym->name);
}

void
emitefun(void)
{
	puts("}");
}

void
emitret(Type *tp)
{
	fputs("\ty", stdout);
	emittype(tp);
}

void
emitlabel(Symbol *sym)
{
	printf("L%d\n", sym->id);
}

void
emitbloop(void)
{
	puts("\td");
}

void
emiteloop(void)
{
	puts("\tb");
}

void
emitjump(Symbol *sym, Node *np)
{
	printf("\tj\tL%d", sym->id);
	if (!np)
		putchar('\n');
	else
		emitexp(np);
}

void
emitswitch(short nr, Node *np)
{
	printf("\teI\t#%0x", nr);
	emitexp(np);
}

void
emitcase(Symbol *sym, Node *np)
{
	fputs("\tw\t", stdout);
	printf("L%d", sym->id);
	emitexp(np);
}

void
emitdefault(Symbol *sym)
{
	fputs("\tf\t", stdout);
	emitlabel(sym);
}

void
emitfield(Node *np)
{
	Node *child;

	child = np->childs[0];
	(*child->code)(child);
	putchar('\t');
	emitvar(np->u.sym);
}

enum {
	SYM, TYP, OP
};


/*TODO: Remove type of union unode */

struct kindnode {
	uint8_t nchilds;
	char unode;
	void (*code)(Node *);
} kindnodes [] = {
	[CAST]= {
		.nchilds = 1,
		.code = emitcast
	},
	[FIELD] = {  /*TODO: Create a node for the symbol */
		.nchilds = 1,
		.unode = SYM,
		.code = emitfield
	},
	[UNARY] = {
		.nchilds = 1,
		.unode = OP,
		.code = emitunary
	},
	[BINARY] = {
		.nchilds = 2,
		.unode = OP,
		.code = emitbin
	},
	[SIZEOFCODE] = {
		.nchilds = 0,
		.unode = TYP,
		.code = emitsizeof
	},
	[SYMBOL] = {
		.nchilds = 0,
		.unode = SYM,
		.code = emitsym
	},
	[TERNARY] = {
		.nchilds = 3,
		.code = emitternary
	}
};

Node *
node(char kind, Type *tp, ...)
{
	uint8_t nchilds, i;
	va_list va;
	struct kindnode *kp;
	Node *np;

	va_start(va, tp);
	kp = &kindnodes[kind];
	nchilds = kp->nchilds;
	np = xmalloc(sizeof(*np) + nchilds * sizeof(np));

	np->code = kp->code;
	np->nchilds = nchilds;
	np->type = tp;
	np->typeop = tp->op;
	np->b.symbol = np->b.lvalue = 0;

	switch (kp->unode) {
	case TYP:
		np->u.type = va_arg(va, Type *);
		break;
	case SYM:
		np->u.sym = va_arg(va, Symbol *);
		np->b.symbol = 1;
		np->b.constant = 1;
		break;
	case OP:
		np->u.op = va_arg(va, int);
		break;
	}

	for (i = 0; i < nchilds; ++i)
		np->childs[i] = va_arg(va, Node *);

	va_end(va);
	return np;
}
