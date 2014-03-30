
#include <stdint.h>
#include <stdio.h>

#include "cc.h"

Node *
node(Inst code, Type *tp, union unode u, uint8_t nchilds)
{
	Node *np = xmalloc(sizeof(*np) + nchilds * sizeof(np));

	np->code = code;
	np->type = tp;
	np->u = u;

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
bincode(char op, Node *np1, Node *np2)
{
	Node *np = node(emitbin, np1->type, OP(op), 2);
	np->childs[0] = np1;
	np->childs[1] = np2;
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
		c = 'R';
	else
		c = 'A';
	printf("\t%c%d", c, sym->id);
}

void
emitunary(Node *np)
{
}

void
emitbin(Node *np)
{
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
