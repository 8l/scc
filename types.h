#ifndef TYPES_H_
#define TYPES_H_

#ifndef __bool_true_false_are_defined
#  include <stdbool.h>
#endif

struct ctype;

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
		};
		size_t nelem;		      /* size of array */
	};
};


extern struct type tchar, tshort, tint, tulong, tllong, tvoid, tkeyword;
extern struct type tfloat, tdouble, tldouble, tlong;

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


#define ARY		1
#define PTR		2
#define FTN		3

struct type *mktype(register struct type *base, unsigned  char op);

extern struct type *decl_type(struct type *t);
void pushtype(unsigned char mod);

#ifndef NDEBUG
extern void ptype(register struct type *t);
#else
#  define ptype(t)
#endif


extern struct type *btype(struct type *tp, unsigned char tok);

#endif
