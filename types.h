#ifndef TYPES_H_
#define TYPES_H_


struct type {
	unsigned op    : 5;
	struct type *base;
	struct type *ary;		      /* array */
	struct type *ptr;		      /* pointer */
	struct type *ftn;		      /* function */
	struct type *cnst;		      /* const */
	struct type *vltl;		      /* volatile */
	struct type *rstr;		      /* restricted */

	union  {
		struct {
			unsigned btype : 4;
			unsigned sign : 1;
		};
		size_t nelem;		      /* size of array */
	};
};


extern struct type tschar, tuchar, tshort, tushort, tint, tuint;
extern struct type tfloat, tdouble, tldouble, tlong;
extern struct type tulong, tllong, tullong, tvoid, tkeyword;

#define T_SCHAR   (&tschar)
#define T_UCHAR   (&tuchar)
#define T_SHORT   (&tshort)
#define T_USHORT  (&tushort)
#define T_INT     (&tint)
#define T_UINT    (&tuint)
#define T_FLOAT   (&tfloat)
#define T_DOUBLE  (&tdouble)
#define T_LDOUBLE (&tdouble)
#define T_LONG    (&tlong)
#define T_ULONG   (&tulong)
#define T_LLONG   (&tllong)
#define T_ULLONG  (&tullong)
#define T_VOID    (&tvoid)
#define T_KWD     (&tkeyword)

#define ARY		1
#define PTR		2
#define FTN		3
#define T_CONST		1
#define T_RESTRICTED	2
#define T_VOLATILE	4

#define QLF(x)   (VOLATILE - x + 1)


struct type *mktype(register struct type *base, unsigned  char op);
/* ANSI C says minimum maximum for indirection level is 12 */
#define PTRLEVEL_MAX 12

extern struct type *decl_type(struct type *t);
extern void pushtype(char op);

#ifndef NDEBUG
extern void ptype(register struct type *t);
#else
#  define ptype(t)
#endif


extern unsigned char isfunction(struct type *t);

#endif
