#ifndef TYPES_H_
#define TYPES_H_


struct type {
	unsigned char op;
	struct type *base;
	struct type *ary;               /* array */
	struct type *ptr;		/* pointer */
	struct type *ftn;		/* function */
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
extern struct type tulong, tllong, tullong, tvoid;

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


#define ARY             1
#define PTR             2
#define FTN             3
#define T_CONST           8
#define T_RESTRICTED     16
#define T_VOLATILE       32

struct type *mktype(register struct type *base, unsigned  char op);

#endif
