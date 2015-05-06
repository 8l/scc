
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/cc.h"
#include "cc1.h"

static void emitbin(void *), emitunary(void *), emitternary(void *),
     emitcast(void *), emitsym(void *), emitfield(void *),
     emitsizeof(void *);

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
	[OCOMMA] = ","
};

void (*opcode[])(void *) = {
	[OADD] = emitbin,
	[OSUB] = emitbin,
	[OMUL] = emitbin,
	[OINC] = emitunary,
	[ODEC] =  emitunary,
	[OSIZE] = emitsizeof,
	[OPTR] = emitunary,
	[OMOD] = emitbin,
	[ODIV] = emitbin,
	[OSHL] = emitbin,
	[OSHR]  = emitbin,
	[OLT] = emitbin,
	[OGT] = emitbin,
	[OGE] = emitbin,
	[OLE] =  emitbin,
	[OEQ] = emitbin,
	[ONE] = emitunary,
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
	[OADDR] = emitunary,
	[ONEG] = emitunary,
	[OCPL] = emitunary,
	[OAND] = emitbin,
	[OOR] = emitbin,
	[OCOMMA] = emitbin,
	[OCAST] = emitcast,
	[OSYM] = emitsym,
	[OASK] = emitternary,
	[OFIELD]= emitfield
};

void
freetree(Node *np)
{
	Node **p;

	if (!np)
		return;
	freetree(np->left);
	freetree(np->rigth);
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
emitsym(void *arg)
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
emitcast(void *arg)
{
	Node *np = arg, *lp = np->left;

	(*opcode[lp->op])(lp);
	printf("\t%c%c", lp->type->letter, np->type->letter);
}

void
emitunary(void *arg)
{
	Node *lp, *np = arg;
	char letter;

	letter = np->type->letter;
	lp = np->left;
	(*opcode[lp->op])(lp);
	printf("\t%s%c", optxt[np->op], letter);
}

void
emitbin(void *arg)
{
	Node *lp, *rp, *np = arg;

	lp = np->left;
	rp = np->rigth;
	(*opcode[lp->op])(lp);
	(*opcode[rp->op])(rp);
	printf("\t%s%c", optxt[np->op], np->type->letter);
}

void
emitternary(void *arg)
{
	Node *cond, *ifyes, *ifno, *np = arg;

	cond = np->left;
	ifyes = np->rigth->left;
	ifno = np->rigth->rigth;
	(*opcode[cond->op])(cond);
	(*opcode[ifyes->op])(ifyes);
	(*opcode[ifno->op])(ifno);
	printf("\t?%c", np->type->letter);
}

void
emitsizeof(void *arg)
{
	Node *np = arg;
	printf("\t#%c", np->left->type->letter);
}

void
emitexp(Node *np)
{
	if (np)
		(*opcode[np->op])(np);
	putchar('\n');
}

void
emitprint(Node *np)
{
	(*opcode[np->op])(np);
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
emitjump(Symbol *sym)
{
	printf("\tj\tL%d\n", sym->id);
}

void
emitbranch(Symbol *sym)
{
	printf("\tj\tL%d", sym->id);
}

void
emitswitch(short nr)
{
	printf("\teI\t#%0x", nr);
}

void
emitcase(Symbol *sym)
{
	fputs("\tw\t", stdout);
	printf("L%d", sym->id);
}

void
emitdefault(Symbol *sym)
{
	fputs("\tf\t", stdout);
	emitlabel(sym);
}

void
emitfield(void *arg)
{
	Node *np = arg, *lp = np->left;

	(*opcode[lp->op])(lp);
	putchar('\t');
	emitvar(np->sym);
}

Node *
node(uint8_t op, Type *tp, Node *left, Node *rigth)
{
	Node *np;

	np = xmalloc(sizeof(*np));
	np->op = op;
	np->type = tp;
	np->sym = NULL;
	np->constant = np->symbol = np->lvalue = 0;
	np->left = left;
	np->rigth = rigth;
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
