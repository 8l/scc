
#include <stdint.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cc.h"
#include "symbol.h"
#include "tokens.h"

#define NR_SYM_HASH 32

uint8_t curctx;
uint8_t namespace = NS_KEYWORD + 1 ;

static struct symtab {
	struct symbol *head;
	struct symbol *htab[NR_SYM_HASH];
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
	register struct symbol *sym;

	tbl = &symtab[ns];
	for (sym = tbl->head; sym; sym = sym->next) {
		if (sym->ctx <= curctx)
			break;
		tbl->htab[hash(sym->name)] = sym->hash;
		free(sym->name);
		free(sym);
	}
}

struct node *
context(struct node * (*fun)(void))
{
	uint8_t ns;
	struct node *np;

	ns = namespace;
	++curctx;
	np = fun();
	--curctx;
	namespace = ns;

	freesyms(NS_IDEN);
	freesyms(NS_TAG);

	return np;
}

struct symbol *
lookup(register char *s, uint8_t ns)
{
	extern union yystype yylval;
	static struct symtab *tbl;
	register struct symbol *sym;

	if (ns == NS_IDEN && (sym = yylval.sym) && !strcmp(sym->name, s))
		return sym;

	tbl = &symtab[(ns >= NR_NAMESPACES) ? NS_IDEN : ns];
	for (sym = tbl->htab[hash(s)]; sym; sym = sym->hash) {
		register char *t = sym->name;
		if (sym->ns == ns && t && *t == *s && !strcmp(t, s))
			return sym;
	}

	return NULL;
}

struct symbol *
install(char *s, uint8_t ns)
{
	register struct symbol *sym;
	register struct symbol **t;
	struct symtab *tbl;

	if (ns == NS_KEYWORD)
		ns = NS_IDEN;
	else if (s != NULL)
		s = xstrdup(s);

	sym = xcalloc(1, sizeof(*sym));
	sym->name = s;
	sym->ctx = curctx;
	sym->token = IDEN;
	sym->ns = ns;
	tbl = &symtab[(ns >= NR_NAMESPACES) ? NS_IDEN : ns];
	sym->next = tbl->head;
	tbl->head = sym;

	if (s != NULL) {
		t = &tbl->htab[hash(s)];
		sym->hash = *t;
		*t = sym;
	}

	return sym;
}
