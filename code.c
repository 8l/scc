
#include <stdint.h>
#include <stdio.h>

#include "cc.h"

void
emitsym(struct symbol *sym)
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
emitfun(struct symbol *sym)
{
	printf("X%s\n", sym->name);
}

void
emitframe(struct symbol *sym)
{
	puts("{");
}

void
emitret(struct symbol *sym)
{
	puts("}");
}
