
#include <stdlib.h>
#include <string.h>

#include "symbol.h"

#define xmalloc malloc
#define xstrdup strdup
#define NR_SYM_HASH 32

struct symhash {
	struct symbol buf[NR_SYM_HASH];
	struct symbol *top;
};


unsigned char nested_level;
static struct symhash iden_hash;
static struct symctx ctx_base;
static struct symctx *ctx_head = &ctx_base;

static void del_hash_ctx(struct symhash *htable, struct symbol *lim)
{
	register struct symbol *bp, *next, *prev;

	for (bp = htable->top; bp && bp != lim; bp = bp->next) {
		next = bp->h_next, prev = bp->h_prev;
		prev->h_next = next;
		next->h_prev = prev;
		free(bp->str);
		free(bp);
	}
}

void new_ctx(struct symctx *ctx)
{
	++nested_level;
	ctx->next = ctx_head;
	ctx_head = ctx;
	ctx->iden = iden_hash.top;
}

void del_ctx(void)
{
	--nested_level;
	del_hash_ctx(&iden_hash, ctx_head->next->iden);
}

struct symbol *install(const char *s, unsigned char key)
{
	static struct symbol *head;
	register struct symbol *sym, *next;

	sym = xmalloc(sizeof(*sym));

	sym->next = iden_hash.top;
	iden_hash.top = sym;

	if (s) {
		sym->str = xstrdup(s);
		head = &iden_hash.buf[key & NR_SYM_HASH-1];
		next = head->h_next;
		sym->h_next = next;
		sym->h_prev = next->h_prev;
		head->h_next = sym;
		next->h_prev = sym;
	} else {
		sym->h_next = sym->h_prev = NULL;
		sym->str = NULL;
	}
	return sym;
}

struct symbol *lookup(char *s, unsigned char key)
{
	register struct symbol *bp, *head;

	head = &iden_hash.buf[key & NR_SYM_HASH-1];
	for (bp = head->h_next; bp != head; bp = bp->h_next) {
		if (!strcmp(bp->str, s))
			return bp;
	}
	return NULL;
}

void init_symbol(void)
{
	register struct symbol *bp;

	for (bp = iden_hash.buf; bp < &iden_hash.buf[NR_SYM_HASH]; ++bp)
		bp->h_next = bp->h_prev = bp;
}

unsigned char hashfun(register const char *s)
{
	register unsigned char h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h;
}
