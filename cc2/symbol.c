/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/symbol.c";
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"

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
		/*
		 * Symbols are inserted in the hash in the inverted
		 * order they are found in locals and it is impossible
		 * to have a global over a local, because a local is
		 * any symbol defined in the body of a function,
		 * even if it has extern linkage.
		 * For this reason when we reach a symbol in the
		 * locals list we know that it is the head of it
		 * collision list and we can remove it assigning
		 * it h_next to the hash table position
		 */
		if (sym->id != TMPSYM)
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

	if (id >= USHRT_MAX)
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
		if (id != TMPSYM) {
			sym->h_next = *htab;
			*htab = sym;
		}
	}
	return sym;
}
