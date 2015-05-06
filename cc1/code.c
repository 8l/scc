
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/cc.h"
#include "cc1.h"

static void emitbin(uint8_t, void *), emitunary(uint8_t, void *),
            emitternary(uint8_t, void *), emitcast(uint8_t, void *),
            emitsym(uint8_t, void *), emitfield(uint8_t, void *),
            emitsizeof(uint8_t, void *), emitexp(uint8_t op, void *arg);

char *optxt[] = {
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
	[OCOMMA] = ",",
	[OLABEL] = "L%d\n",
	[ODEFAULT] = "\tf\tL%d\n",
	[OCASE] = "\tw\tL%d",
	[OSTRUCT] = "S%d\t(\n",
	[OJUMP] = "\tj\tL%d\n",
	[OBRANCH] = "\tj\tL%d",
};

void (*opcode[])(uint8_t, void *) = {
	[OADD] = emitbin,
	[OSUB] = emitbin,
	[OMUL] = emitbin,
	[OINC] = emitbin,
	[ODEC] =  emitbin,
	[OSIZE] = emitsizeof,
	[OPTR] = emitbin,
	[OMOD] = emitbin,
	[ODIV] = emitbin,
	[OSHL] = emitbin,
	[OSHR]  = emitbin,
	[OLT] = emitbin,
	[OGT] = emitbin,
	[OGE] = emitbin,
	[OLE] =  emitbin,
	[OEQ] = emitbin,
	[ONE] = emitbin,
	[OBAND] = emitbin,
	[OBXOR]  = emitbin,
	[OBOR] = emitbin,
	[OASSIGN] = emitbin,
	[OA_MUL] = emitbin,
	[OA_DIV] = emitbin,
	[OA_MOD] = emitbin,
	[OA_ADD] = emitbin,
	[OA_SUB] = emitbin,
	[OA_SHL] = emitbin,
	[OA_SHR] = emitbin,
	[OA_AND] = emitbin,
	[OA_XOR] = emitbin,
	[OA_OR] = emitbin,
	[OADDR] = emitbin,
	[ONEG] = emitbin,
	[OCPL] = emitbin,
	[OAND] = emitbin,
	[OOR] = emitbin,
	[OCOMMA] = emitbin,
	[OCAST] = emitcast,
	[OSYM] = emitsym,
	[OASK] = emitternary,
	[OFIELD]= emitfield,
	[OEXPR] = emitexp
};

void
freetree(Node *np)
{
	if (!np)
		return;
	freetree(np->left);
	freetree(np->right);
	free(np);
}

static void
emitnode(Node *np)
{
	if (np)
		(*opcode[np->op])(np->op, np);
}

void
emit(uint8_t op, void *arg)
{
	extern uint8_t failure;

	if (failure)
		return;
	(*opcode[op])(op, arg);
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
		c = 'K';
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
	Symbol *sym = np->sym;

	if (np->type == inttype) {
		printf("#%c%x", np->type->letter, sym->u.i);
	} else {
		putchar('"');
		for (bp = sym->u.s; c = *bp; ++bp)
			printf("%02x", (unsigned) c);
	}
}

void
emitestruct(void)
{
	puts(")");
}

void
emitsym(uint8_t op, void *arg)
{
	Node *np = arg;
	putchar('\t');
	(np->constant) ? emitconst(np) : emitvar(np->sym);
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
emitcast(uint8_t op, void *arg)
{
	Node *np = arg, *lp = np->left;

	emitnode(lp);
	printf("\t%c%c", lp->type->letter, np->type->letter);
}

void
emitbin(uint8_t op, void *arg)
{
	Node *np = arg;

	emitnode(np->left);
	emitnode(np->right);
	printf("\t%s%c", optxt[op], np->type->letter);
}

void
emitternary(uint8_t op, void *arg)
{
	Node *cond, *ifyes, *ifno, *np = arg;

	cond = np->left;
	ifyes = np->right->left;
	ifno = np->right->right;
	emitnode(cond);
	emitnode(ifyes);
	emitnode(ifno);
	printf("\t?%c", np->type->letter);
}

void
emitsizeof(uint8_t op, void *arg)
{
	Node *np = arg;
	printf("\t#%c", np->left->type->letter);
}

static void
emitexp(uint8_t op, void *arg)
{
	Node *np = arg;

	emitnode(np);
	putchar('\n');
}

void
emitprint(Node *np)
{
	emitnode(np);
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
emitsymid(uint8_t op, Symbol *sym)
{
	printf(optxt[op], sym->id);
}

void
emitswitch(short nr)
{
	printf("\teI\t#%0x", nr);
}

void
emitfield(uint8_t op, void *arg)
{
	Node *np = arg;

	emitnode(np->left);
	putchar('\t');
	emitvar(np->sym);
}

Node *
node(uint8_t op, Type *tp, Node *left, Node *right)
{
	Node *np;

	np = xmalloc(sizeof(*np));
	np->op = op;
	np->type = tp;
	np->sym = NULL;
	np->constant = np->symbol = np->lvalue = 0;
	np->left = left;
	np->right = right;
	return np;
}

Node *
symbol(Symbol *sym)
{
	Node *np;

	np = node(OSYM, sym->type, NULL, NULL);
	np->symbol = 1;
	np->constant = 1;
	np->sym = sym;
	return np;
}
