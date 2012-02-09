

#include <stddef.h>

#include "symbol.h"

#define NR_SYM_HASH 32

struct symhash {
	struct symbol *buf[NR_SYM_HASH];
	struct symbol *top;
};


struct symctx {
	struct symbol *siden;
	struct symbol *sstruct;
	struct symbol *sgoto;
	struct symctx *next;
};



static struct symctx global_ctx;
static struct symctx *ctxp = &global_ctx;
struct symhash siden, sgoto, sstruct;





void new_ctx(struct symctx *ctx)
{
	ctx->siden = siden.top;
	ctx->sstruct = sstruct.top;
	ctx->sgoto = sgoto.top;
	ctx->next = ctxp;
	ctxp = ctx;
}


/*
 * WARNING: This function is not portable and waits that incremental calls
 * to alloca return decremented address
 */
static void del_hash_ctx(struct symhash *h, struct symbol *const top)
{
	register struct symbol **bp;
	static struct symbol **lim;

	lim = h->buf + NR_SYM_HASH;
	for (bp = h->buf; bp < lim; bp++) {
		register struct symbol *aux;
		for (aux = *bp; aux < top; *bp = aux = aux->next)
			if (aux == h->top)
				h->top = aux;
	}
}


void del_ctx(void)
{
	del_hash_ctx(&siden, ctxp->siden);
	del_hash_ctx(&sstruct, ctxp->sstruct);
	del_hash_ctx(&sgoto, ctxp->sgoto); /* TODO: correct handling in goto */
}




struct symbol *pushsym(struct symhash *h, struct symbol *sym, unsigned char hash)
{
	static unsigned char key;

	key = hash % NR_SYM_HASH;
	h->top = sym;
	sym->next = h->buf[key];
	return h->buf[key] = sym;
}




struct symbol *findsym(struct symhash *h, char *s, unsigned char hash)
{
	register struct symbol *bp;

	for (bp = h->buf[hash % NR_SYM_HASH]; bp; bp = bp->next) {
		if (!strcmp(bp->str, s))
			return bp;
	}
	return NULL;
}
