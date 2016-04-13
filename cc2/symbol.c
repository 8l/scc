
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"

#include "arch.h"
#include "cc2.h"

#define NR_SYMHASH  64

static Symbol *symtab[NR_SYMHASH];
static Symbol *locals;
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
	locals = NULL;
}

Symbol *
getsym(int id)
{
	Symbol **htab, *sym;

	htab = &symtab[id & NR_SYMHASH-1];
	for (sym = *htab; sym; sym = sym->h_next) {
		if (sym->id > 0 && sym->id == id)
			break;
	}
	if (!sym) {
		sym = xcalloc(1, sizeof(*sym));
		sym->id = id;
		if (!infunction) {
			sym->next = NULL;
		} else {
			sym->next = locals;
			locals = sym;
		}
		sym->h_next = *htab;
		*htab = sym;
	}
	return sym;
}
