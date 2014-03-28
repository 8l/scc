
#include <stdint.h>
#include <stdio.h>

#include "cc.h"

void
emitsym(Symbol *sym)
{
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
