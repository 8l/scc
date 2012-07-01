
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

#define T_CHAR   (&tschar)
#define T_SHORT   (&tshort)
#define T_INT     (&tint)
#define T_FLOAT   (&tfloat)
#define T_DOUBLE  (&tdouble)
#define T_LDOUBLE (&tdouble)
#define T_LONG    (&tlong)
#define T_LLONG   (&tllong)
#define T_VOID    (&tvoid)
#define T_BOOL    (&tbool)

enum namespace {
	NS_IDEN,
	NS_KEYWORD,
	NS_STRUCT,
	NS_LABEL,
	NS_TYPEDEF
};

struct ctype {
	bool c_type : 1;
	bool c_extrn : 1;
	bool c_static : 1;
	bool c_auto : 1;
	bool c_reg : 1;
	bool c_const : 1;
	bool c_vol : 1;
	bool c_unsigned : 1;
	struct type *base;
};

struct type {
	unsigned char op;
	struct type *base;

	union  {
		size_t nelem;		      /* size of array */
	};
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

extern struct type tchar, tshort, tint, tulong, tllong, tvoid, tkeyword;
extern struct type tfloat, tdouble, tldouble, tlong;


extern struct type *decl_type(struct type *t);
extern void pushtype(unsigned char mod);
extern struct type *btype(struct type *tp, unsigned char tok);
extern void new_ctx(struct symctx *ctx);
extern void del_ctx(void);
extern struct symbol *install(const char *s, unsigned char key);
extern struct symbol *lookup(char *s, unsigned char key);
extern unsigned char hashfun(register const char *s);
extern void ctype(struct ctype *cp, unsigned char mod);

#ifndef NDEBUG
extern void ptype(register struct type *t);
#else
#  define ptype(t)
#endif

#endif
