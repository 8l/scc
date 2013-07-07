
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

#define CTX_OUTER 1
#define CTX_FUNC  2
#define CTX_ANY   0

enum namespace {
	NS_IDEN,
	NS_KEYWORD,
	NS_STRUCT,
	NS_LABEL,
	NS_TYPEDEF,
	NS_ANY
};

struct ctype {
	unsigned type : 5;
	bool c_typedef : 1;
	bool c_extern : 1;
	bool c_static : 1;
	bool c_auto : 1;
	bool c_register : 1;
	bool c_const : 1;
	bool c_volatile : 1;
	bool c_restrict : 1;
	bool c_unsigned : 1;
	bool c_signed : 1;
	unsigned len;
	struct ctype *base;
	unsigned char refcnt;
};

struct symbol {
	struct ctype *ctype;
	unsigned char ctx;
	unsigned char ns;
	char *name;
	struct {
		union {
			unsigned char tok;  /* used in keywords */
			short val;	/* used in integer constant */
		};
	};
	struct symbol *next;
	struct symbol *hash;
};


extern struct ctype *decl_type(struct ctype *t);
extern void pushtype(unsigned mod);
extern struct ctype *btype(struct ctype *tp, unsigned char tok);
extern void new_ctx(void);
extern void del_ctx(void);
extern void freesyms(void);
extern struct symbol *lookup(const char *s, char ns);
extern struct symbol *find(const char *s, char ns);
extern void insert(struct symbol *sym, unsigned char ctx);
extern struct ctype *storage(struct ctype *tp, unsigned char mod);
extern struct ctype *newctype(void);
extern void delctype(struct ctype *tp);

#ifndef NDEBUG
extern void ptype(register struct ctype *t);
#else
#  define ptype(t)
#endif

#endif
