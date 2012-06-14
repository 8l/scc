
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

enum namespace {
	NS_IDEN,
	NS_KEYWORD,
	NS_STRUCT,
	NS_LABEL,
	NS_TYPEDEF
};

struct type;
struct symbol;

struct ctype {
	bool c_typedef : 1;
	bool c_extern : 1;
	bool c_static : 1;
	bool c_auto : 1;
	bool c_register : 1;
	bool c_const : 1;
	bool c_volatile : 1;
	struct type *base;
};

struct symbol {
	struct ctype ctype;
	unsigned char ns;
	union {
		struct {
			char *str;
			union {
				unsigned char level;/* used in usual symbols */
				unsigned char tok;  /* used in keywords */
			};
		};
		short val;	/* used in integer constant */
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
