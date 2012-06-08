
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H


struct type;

struct symbol {
	char *str;
	unsigned char level;
	struct type *type;
	struct symbol *next;
	struct symbol *h_next, *h_prev;
};


struct symctx {
	struct symbol *iden;
	struct symctx *next;
};


extern void new_ctx(struct symctx *ctx);
extern void del_ctx(void);
extern struct symbol *addsym(const char *s, unsigned char key);
extern struct symbol *lookupsym(char *s, unsigned char key);

#endif
