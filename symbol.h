
#ifndef SYMBOL_H
#define SYMBOL_H

#define CTX_OUTER 0
#define CTX_FUNC  1

#define isfun(t) 0

enum {
	NS_IDEN = 0,
	NS_LABEL,
	NS_TAG,
	NR_NAMESPACES,
	NS_KEYWORD,
	NS_FREE
};

struct funpars;
struct symbol;

struct ctype {
	uint8_t op;           /* type builder operator */
	short size;           /* size of variables */
	short nelem;          /* number of elements in arrays */
	unsigned defined : 1; /* type defined (is not a forward reference) */
	unsigned cplex : 1;   /* complex specifier */
	unsigned imag : 1;
	unsigned sign : 1;    /* sign type */
	struct symbol *sym;   /* symbol of the tag identifier */
	struct ctype *type;   /* base type */
	struct ctype *next;   /* next element in the hash */
	struct funpars *pars; /* function parameters */
};

struct funpars {
	struct ctype *type;
	struct funpars *next;
};

struct symbol {
	char *name;
	struct ctype *type;
	uint8_t ctx;
	uint8_t token;
	uint8_t ns;
	union {
		char c;
		short offset;
	} u;
	struct symbol *next;
	struct symbol *hash;
};

extern void freesyms(uint8_t ns);

extern struct ctype *qualifier(struct ctype *tp, uint8_t qlf),
	*ctype(int8_t type, int8_t sign, int8_t size, int8_t cplex),
	*mktype(struct ctype *tp,
	        uint8_t op, struct symbol *tag, uint16_t nelem);

extern struct symbol
	*lookup(char *s, unsigned char ns),
	*install(char *s, unsigned char ns);

extern struct node *context(struct node * (*fun)(void));

extern struct ctype *voidtype,
	*uchartype,   *chartype,
	*uinttype,    *inttype,
	*ushortype,   *shortype,
	*longtype,    *ulongtype,
	*ullongtype,  *llongtype,
	*floattype,   *cfloattype,  *ifloattype,
	*doubletype,  *cdoubletype, *idoubletype,
	*ldoubletype, *cldoubletype,*ildoubletype;

#endif
