
#include <stdint.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cc.h>
#include "cc1.h"

#define NR_SYM_HASH 32

uint8_t curctx;
short symid;

static struct symtab {
	Symbol *head;
	Symbol *htab[NR_SYM_HASH];
} symtab [NR_NAMESPACES];

static inline uint8_t
hash(register const char *s)
{
	register uint8_t h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h & NR_SYM_HASH - 1;
}

void
freesyms(uint8_t ns)
{
	static struct symtab *tbl;
	register Symbol *sym, *next;

	tbl = &symtab[ns];
	for (sym = tbl->head; sym; sym = next) {
		if (sym->ctx <= curctx)
			break;
		if (ns == NS_LABEL && !sym->s.isdefined)
			error("label '%s' is not defined", sym->name);
		tbl->htab[hash(sym->name)] = sym->hash;
		next = tbl->head = sym->next;
		free(sym->name);
		free(sym);
	}
}

Type *
aggregate(Type * (*fun)(void))
{
	Type *tp;

	++curctx;
	tp = (*fun)();
	--curctx;
	freesyms(NS_IDEN);
	return tp;
}

void
context(Symbol *lbreak, Symbol *lcont, Caselist *lswitch)
{
	++curctx;
	compound(lbreak, lcont, lswitch);
	--curctx;
	freesyms(NS_IDEN);
	freesyms(NS_TAG);
}

Symbol *
lookup(register char *s, uint8_t ns)
{
	register Symbol *sym;

	for (sym = symtab[ns].htab[hash(s)]; sym; sym = sym->hash) {
		if (!strcmp(sym->name, s))
			return sym;
	}

	return NULL;
}

Symbol *
install(char *s, uint8_t ns)
{
	register Symbol *sym, **t;
	struct symtab *tbl;

	sym = xcalloc(1, sizeof(*sym));
	sym->name = xstrdup(s);
	sym->ctx = curctx;
	sym->token = IDEN;
	sym->id = symid++;
	sym->s.isdefined = 1;
	tbl = &symtab[ns];
	sym->next = tbl->head;
	tbl->head = sym;


	t = &tbl->htab[hash(s)];
	sym->hash = *t;
	*t = sym;

	return sym;
}
