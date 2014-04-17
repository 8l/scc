
#include <stdint.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cc1.h"

#define NR_SYM_HASH 32

uint8_t curctx;
uint8_t namespace = NS_FREE + 1;
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
	register Symbol *sym;

	tbl = &symtab[ns];
	for (sym = tbl->head; sym; sym = sym->next) {
		if (sym->ctx <= curctx)
			break;
		tbl->htab[hash(sym->name)] = sym->hash;
		free(sym->name);
		free(sym);
	}
}

void
context(void (*fun)(void))
{
	uint8_t ns;

	ns = namespace;
	++curctx;
	fun();
	--curctx;
	namespace = ns;

	freesyms(NS_IDEN);
	freesyms(NS_TAG);
}

Symbol *
lookup(register char *s, uint8_t ns)
{
	extern union yystype yylval;
	static struct symtab *tbl;
	register Symbol *sym;

	tbl = &symtab[(ns >= NR_NAMESPACES) ? NS_IDEN : ns];
	for (sym = tbl->htab[hash(s)]; sym; sym = sym->hash) {
		register char *t = sym->name;
		if (sym->ns == ns && t && *t == *s && !strcmp(t, s))
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
	sym->ns = ns;
	sym->id = symid++;
	tbl = &symtab[(ns >= NR_NAMESPACES) ? NS_IDEN : ns];
	sym->next = tbl->head;
	tbl->head = sym;


	t = &tbl->htab[hash(s)];
	sym->hash = *t;
	*t = sym;

	return sym;
}
