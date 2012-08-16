
#include <stdlib.h>
#include <string.h>

#include "cc.h"
#include "symbol.h"

#define NR_SYM_HASH 32

unsigned char curctx;

static struct symbol *htab[NR_SYM_HASH];
static struct symbol *head, *headfun;


static inline unsigned char
hash(register const char *s)
{
	register unsigned char h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h & NR_SYM_HASH - 1;
}

void
new_ctx(void)
{
	++curctx;
}

void
del_ctx(void)
{
	register struct symbol *sym, *next;
	static char *s;

	--curctx;
	for (sym = head; sym; sym = next) {
		if (sym->ctx <= curctx)
			break;
		if ((s = sym->name) != NULL)
			htab[hash(s)] = sym->hash;
		next = sym->next;
		sym->next = headfun;
		headfun = sym;
	}
}

void
freesyms(void)
{
	register struct symbol *sym, *next;

	if (curctx == OUTER_CTX) {
		for (sym = headfun; sym; sym = next) {
			next = sym->next;
			free(sym->name);
			free(sym);
		}
	}
}

struct symbol *
install(const char *s)
{
	register struct symbol *sym;
	register unsigned char key;

	sym = xmalloc(sizeof(*sym));
	sym->ctx = curctx;
	sym->next = head;
	head = sym;

	if (s) {
		sym->name = xstrdup(s);
		key = hash(s);
		sym->hash = htab[key];
		htab[key] = sym;
		sym->ns = NS_IDEN;
	} else {
		sym->hash = NULL;
		sym->name = NULL;
	}
	return sym;
}

struct symbol *
lookup(const char *s)
{
	register struct symbol *sym;
	static  unsigned char l;

	l = strlen(s);
	for (sym = htab[hash(s)]; sym; sym = sym->hash) {
		if (!memcmp(sym->name, s, l))
			break;
	}
	return sym;
}
