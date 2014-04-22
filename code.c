
#include <stdint.h>
#include <stdio.h>

#include "cc1.h"

char *opcodes[] = {
	[OADD] = "+",
	[OSUB] = "-",
	[OMUL] = "*",
	[OARY] = "'",
	[OINC] = ";+",
	[ODEC] =  ";=",
	[OSIZE] = "#",
	[OPTR] = "@",
	[OMOD] = "*",
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
	[OAND] = "m",
	[OOR] = "s",
	[OCOMMA] = ","
};

Node *
node(Inst code, Type *tp, union unode u, uint8_t nchilds)
{
	Node *np = xmalloc(sizeof(*np) + nchilds * sizeof(np));

	np->code = code;
	np->type = tp;
	np->utype = UNQUAL(tp);
	np->typeop = np->utype->op;
	np->u = u;
	np->b.symbol = np->b.lvalue = 0;

	return np;
}

static void
emitsym2(Symbol *sym)
{
	char c;

	if (sym->s.isglobal)
		c = 'G';
	else if (sym->s.isstatic)
		c = 'T';
	else if (sym->s.isregister)
		c = 'Q';
	else
		c = 'A';
	printf("%c%d", c, sym->id);
}

static void
emitconst(Node *np)
{
	printf("#%x", np->u.sym->u.i);
}

void
emitsym(Node *np)
{
	putchar('\t');
	(np->b.constant) ? emitconst(np) : emitsym2(np->u.sym);
}

static void
emittype(Type *tp)
{
	putchar(tp->letter);
}

void
emitdcl(Symbol *sym)
{
	emitsym2(sym);
	putchar('\t');
	emittype(sym->type);
	putchar('\n');
}

void
emitcast(Node *np)
{
	Node *child = np->childs[0];

	(*child->code)(child);
	printf("\t%c%c", np->u.type->letter, np->type->letter);
}

void
emitunary(Node *np)
{
	Node *child;
	char op, letter;

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
	(*np->code)(np);
	putchar('\n');
}

void
emitprint(Node *np)
{
	(*np->code)(np);
	printf("\tk%c\n", np->type->letter);
}

void
emitfun(Symbol *sym)
{
	printf("%c%d\tn%s\n",
	       sym->s.isglobal ? 'X' : 'Y', sym->id, sym->name);
}

void
emitsframe(Symbol *sym)
{
	puts("{");
}

void
emiteframe(Symbol *sym)
{
	puts("}");
}

void
emitret(Type *tp)
{
	fputs("\ty", stdout);
	emittype(tp);
}

Node *
castcode(Node *child, Type *tp)
{
	Node *np = node(emitcast, tp, TYP(child->type), 1);

	np->childs[0] = child;
	return np;
}
	
Node *
unarycode(char op, Type *tp, Node *child)
{
	Node *np = node(emitunary, tp, OP(op), 1);
	np->childs[0] = child;
	return np;
}

Node *
bincode(char op, Type *tp, Node *np1, Node *np2)
{
	Node *np = node(emitbin, tp, OP(op), 2);
	np->childs[0] = np1;
	np->childs[1] = np2;
	return np;
}

Node *
sizeofcode(Type *tp)
{
	if (!tp->defined)
		error("invalid use of indefined type");
	return node(emitsizeof, inttype, TYP(tp), 0);
}

Node *
ternarycode(Node *cond, Node *ifyes, Node *ifno)
{
	Node *np= node(emitternary, ifyes->type, OP(0), 3);
	np->childs[0] = cond;
	np->childs[1] = ifyes;
	np->childs[2] = ifno;
	return np;
}

Node *
symcode(Symbol *sym)
{
	Node *np;

	np = node(emitsym, sym->type, SYM(sym), 0);
	np->b.symbol = 1;
	np->b.constant = 1;
	return np;
}
