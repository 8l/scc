
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H

struct type;

struct symbol {
	struct type *type;
	union {
		struct {	/* used in usual symbols */
			char *str;
			unsigned char level;
		};
		unsigned char tok; /* used in keywords */
		short val;
	};
	struct symbol *next;
	struct symbol *h_next, *h_prev;
};

struct symctx {
	struct symbol *iden;
	struct symctx *next;
};

extern void new_ctx(struct symctx *ctx);
extern void del_ctx(void);
extern struct symbol *install(const char *s, unsigned char key);
extern struct symbol *lookup(char *s, unsigned char key);
extern unsigned char hashfun(register const char *s);

#endif
