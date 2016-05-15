
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"

#include "arch.h"
#include "cc2.h"

#define NR_SYMHASH  64

Symbol *locals;

static Symbol *symtab[NR_SYMHASH], *curlocal;
static int infunction;


void
freesym(Symbol *sym)
{
	free(sym->name);
	free(sym);
}

void
pushctx(void)
{
	infunction = 1;
}

void
popctx(void)
{
	Symbol *sym, *next;

	infunction = 0;
	for (sym = locals; sym; sym = next) {
		next = sym->next;
		symtab[sym->id & NR_SYMHASH-1] = sym->h_next;
		freesym(sym);
	}
	curlocal = locals = NULL;
}

Symbol *
getsym(unsigned id)
{
	Symbol **htab, *sym;
	static unsigned short num;

	if (id > USHRT_MAX)
		error(EBADID);

	htab = &symtab[id & NR_SYMHASH-1];
	for (sym = *htab; sym; sym = sym->h_next) {
		if (sym->id > 0 && sym->id == id)
			break;
	}
	if (!sym) {
		sym = xcalloc(1, sizeof(*sym));
		sym->id = id;
		if ((sym->numid = ++num) == 0)
			error(EIDOVER);
		if (infunction) {
			if (!locals)
				locals = sym;
			if (curlocal)
				curlocal->next = sym;
			curlocal = sym;
		}
		sym->h_next = *htab;
		*htab = sym;
	}
	return sym;
}
