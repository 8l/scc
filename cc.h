#ifndef CC_H
#define CC_H

#ifndef __bool_true_and_false_defined
#include <stdbool.h>
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

extern unsigned linenum;
extern unsigned columnum;
extern const char *filename;


struct user_opt {
	unsigned char implicit;
	unsigned char c99;
	unsigned char mixdcls;
	unsigned char useless;
	unsigned char repeat;
	unsigned char charsign;
};

extern  struct user_opt options;

extern void error(const char *fmt, ...);
extern void die(const char *fmt, ...);
extern void warn(char flag, const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);

#define CTX_OUTER 0
#define CTX_FUNC  1

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
	union {
		struct funpar *pars;  /* function parameters */
		struct field *fields; /* aggregate fields */
	} u;
};

struct field {
	struct symbol *sym;
	struct field *next;
};

struct funpar {
	struct ctype *type;
	struct funpar *next;
};

union value {
	char c;
	int i;
	struct symbol *sym;
	uint8_t ns, token;
	short offset;
};

struct symbol {
	char *name;
	struct ctype *type;
	short id;
	uint8_t ctx;
	uint8_t token;
	uint8_t ns;
	struct {
		bool isglobal : 1;
		bool isstatic : 1;
		bool isauto : 1;
		bool isregister : 1;
	} s;
	union value u;
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

extern void context(void (*fun)(void));

extern struct ctype *voidtype,
	*uchartype,   *chartype,
	*uinttype,    *inttype,
	*ushortype,   *shortype,
	*longtype,    *ulongtype,
	*ullongtype,  *llongtype,
	*floattype,   *cfloattype,  *ifloattype,
	*doubletype,  *cdoubletype, *idoubletype,
	*ldoubletype, *cldoubletype,*ildoubletype;

#define ISQUAL(t)    (isqual((t)->op))
#define UNQUAL(t)    (ISQUAL(t) ? (t)->type : (t))
#define BTYPE(t)     (UNQUAL(t)->op)
#define isfun(op)    ((op) == FTN)
#define isptr(op)    ((op) == PTR)
#define isary(op)    ((op) == ARY)
#define isarith(op)  ((op) & ARITH)
#define isaddr(op)   ((op) & POINTER)
#define isrecord(op) ((op) & RECORD)
#define isqual(op)   ((op) & TQUALIFIER)


#define ARITH         8
#define RECORD        16
#define POINTER       32
#define ATYPE(x)      (ARITH  | (x))
#define RTYPE(x)      (RECORD | (x))
#define PTYPE(x)      (POINTER| (x))

#define FTN           1
#define ENUM          2
#define TYPENAME      3
#define VOID          4

#define FLOAT         ATYPE(1)
#define INT           ATYPE(2)
#define BOOL          ATYPE(3)

#define STRUCT        RTYPE(1)
#define UNION         RTYPE(2)

#define PTR           PTYPE(1)
#define ARY           PTYPE(2)

#define CHAR          (ARY+1)
#define DOUBLE        (ARY+2)
#define SHORT         (ARY+3)
#define LONG          (ARY+4)

#define COMPLEX       (ARY+5)
#define IMAGINARY     (ARY+6)
#define UNSIGNED      (ARY+7)
#define SIGNED        (ARY+8)

#define CONST         (1<<0)
#define VOLATILE      (1<<1)
#define RESTRICT      (1<<2)

#define TYPEDEF       1
#define EXTERN        2
#define STATIC        3
#define AUTO          4
#define REGISTER      5

#define accept(t) ((bool) (yytoken == (t) ? next() : 0))
#define ahead()   yyntoken

enum tokens {
	TQUALIFIER = 128, TYPE, IDEN, SCLASS,
	CONSTANT, SIZEOF,
	INDIR, INC, DEC, SHL, SHR,
	LE, GE, EQ, NE, AND, OR,
	MUL_EQ, DIV_EQ, MOD_EQ, ADD_EQ, SUB_EQ, AND_EQ,
	XOR_EQ, OR_EQ, SHL_EQ, SHR_EQ,
	ELLIPSIS,
	CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO,
	CONTINUE, BREAK, RETURN, EOFTOK, NOTOK
};

union yystype {
	struct symbol *sym;
	uint8_t token;
};

extern union yystype yylval;
extern char yytext[];
extern uint8_t yytoken, yyntoken;

extern uint8_t next(void);
extern void expect(uint8_t tok);
#endif
