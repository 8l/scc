
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

#define CTX_OUTER 0
#define CTX_FUNC  1

enum {
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
	bool forward : 1;
	union {
		unsigned len;
		struct {
			struct symbol *sym;
			unsigned char ns;
		};
	};
	struct ctype *base;
};

struct symbol {
	struct ctype *ctype;
	unsigned char ctx;
	unsigned char ns;
	char *name;
	struct {
		union {
			char c;   /* numerical constants */
			short s;
			int i;
			long *l;
			long long *ll;
			unsigned char label;
		};
	};
	struct symbol *next;
	struct symbol *hash;
};

#define HAS_STORAGE(tp) ((tp)->c_extern || (tp)->c_static ||\
                         (tp)->c_auto || (tp)->c_register || (tp)->c_typedef)

#define HAS_QUALIF(tp)  ((tp)->c_const || (tp)->c_volatile)

extern struct ctype *decl_type(struct ctype *t);
extern void pushtype(unsigned mod);
extern struct ctype *ctype(struct ctype *tp, unsigned char tok);
extern void new_ctx(void);
extern void del_ctx(void);
extern void freesyms(void);
extern struct symbol *lookup(const char *s, signed char ns);
extern void insert(struct symbol *sym, unsigned char ctx);
extern struct ctype *storage(struct ctype *tp, unsigned char mod);
extern struct ctype *newctype(void);
extern void delctype(struct ctype *tp);
extern unsigned char hash(register const char *s);

#ifndef NDEBUG
extern void ptype(register struct ctype *t);
#else
#  define ptype(t)
#endif

#endif
