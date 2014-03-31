
#include <stdint.h>
#include <stdio.h>

#include "cc.h"

char *opcodes[] = {
	[OADD] = "+",
	[OARY] = "'",
	[OPTR] = "@"
};

Node *
node(Inst code, Type *tp, union unode u, uint8_t nchilds)
{
	Node *np = xmalloc(sizeof(*np) + nchilds * sizeof(np));

	np->code = code;
	np->type = tp;
	np->u = u;

	return np;
}

void
emitsym(Node *np)
{
	Symbol *sym = np->u.sym;
	char c;

	if (sym->s.isglobal)
		c = 'G';
	else if (sym->s.isstatic)
		c = 'T';
	else if (sym->s.isregister)
		c = 'Q';
	else
		c = 'A';
	printf("\t%c%d", c, sym->id);
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
emitexp(Node *np)
{
	(*np->code)(np);
	putchar('\n');
}

void
emitfun(Symbol *sym)
{
	printf("X%s\n", sym->name);
}

void
emitframe(Symbol *sym)
{
	puts("{");
}

void
emitret(Symbol *sym)
{
	puts("}");
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
