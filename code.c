
#include <stdint.h>
#include <stdio.h>

#include "symbol.h"

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