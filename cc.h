#ifndef CC_H
#define CC_H

#ifndef __bool_true_and_false_defined
#include <stdbool.h>
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct user_opt {
	unsigned char implicit;
	unsigned char mixdcls;
	unsigned char npromote;
	unsigned char useless;
	unsigned char charsign;
};

extern  struct user_opt options;

extern void error(const char *fmt, ...);
extern void die(const char *fmt, ...);
extern void warn(signed char flag, const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);

/* definitions of types */

#define CTX_OUTER 0
#define CTX_FUNC  1

enum {
	NS_IDEN = 0,
	NS_LABEL,
	NS_TAG,
	NR_NAMESPACES,
	NS_FREE
};

struct funpars;
struct symbol;

struct ctype {
	uint8_t op;           /* type builder operator */
	char letter;          /* letter of the type */
	bool defined : 1; /* type defined (is not a forward reference) */
	bool sign : 1;    /* sign type */
	struct symbol *sym;   /* symbol of the tag identifier */
	struct ctype *type;   /* base type */
	struct ctype *next;   /* next element in the hash */
	union {
		signed char size;
		short nelem;          /* number of elements in arrays */
		struct funpar *pars;  /* function parameters */
		struct field *fields; /* aggregate fields */
	} u;
};

typedef struct ctype Type;

struct field {
	struct symbol *sym;
	struct field *next;
};

struct funpar {
	Type *type;
	struct funpar *next;
};

/* definition of symbols */



struct symbol {
	char *name;
	Type *type;
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
	union {
		int i;
		struct symbol *sym;
		uint8_t ns, token;
	} u;
	struct symbol *next;
	struct symbol *hash;
};

typedef struct symbol Symbol;

extern void freesyms(uint8_t ns);

extern Type *qualifier(Type *tp, uint8_t qlf),
	*ctype(int8_t type, int8_t sign, int8_t size),
	*mktype(Type *tp,
	        uint8_t op, Symbol *tag, uint16_t nelem);

extern Symbol
	*lookup(char *s, unsigned char ns),
	*install(char *s, unsigned char ns);

extern void context(void (*fun)(void));

extern Type *typename(void);

extern Type *voidtype, *pvoidtype, *booltype,
	*uchartype,   *chartype,
	*uinttype,    *inttype,
	*ushortype,   *shortype,
	*longtype,    *ulongtype,
	*ullongtype,  *llongtype,
	*floattype,   *doubletype,  *ldoubletype;

#define ISQUAL(t)    (isqual((t)->op))
#define UNQUAL(t)    (ISQUAL(t) ? (t)->type : (t))
#define BTYPE(t)     (UNQUAL(t)->op)
#define isaddr(op)   ((op) & POINTER)
#define isrecord(op) ((op) & RECORD)
#define isqual(op)   ((op) & TQUALIFIER)
#define isconst(op) (((op) & (TQUALIFIER|CONST)) == \
                                     (TQUALIFIER|CONST))


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

#define accept(t) ((yytoken == (t)) ? next() : 0)
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
	Symbol *sym;
	uint8_t token;
};

extern union yystype yylval;
extern char yytext[];
extern uint8_t yytoken, yyntoken;

extern uint8_t next(void);
extern void expect(uint8_t tok);


typedef struct node {
	void (*code)(struct node *);
	Type *type;
	struct {
		bool lvalue : 1;
	} b;
	union unode {
		Symbol *sym;
		Type *type;
		char op;
	} u;
	struct node *childs[];
} Node;

typedef void (*Inst)(Node *); /* TODO: remove this typedef */

enum {
	OCAST = 1, OPTR, OADD, OARY, OSIZE, OMUL, OSUB,
	OINC, ODEC, OPINC, OPDEC, ODIV, OMOD, OSHL, OSHR,
	OBAND, OBXOR, OBOR, OASSIGN, OA_MUL, OA_DIV,
	OA_MOD, OA_ADD, OA_SUB, OA_SHL, OA_SHR,
	OA_AND, OA_XOR, OA_OR, OADDR,ONEG, OCPL, OEXC,
	OCOMMA,
	/*
	  * Complementary relational operators only differ in less
	 * significant bit
	 */
	OEQ = 0x40, ONE, OLT, OGE, OLE, OGT, OAND, OOR
};

extern void
	emitdcl(Symbol *), emitsframe(Symbol *), emiteframe(Symbol *),
	emitsym(Node *), emitunary(Node *),
	emitbin(Node *), emitexp(Node *), emitconst(Node *np);

extern Node
	*node(Inst code, Type *tp, union unode u, uint8_t nchilds),
	*unarycode(char op, Type *tp, Node *child),
	*bincode(char op, Type *tp, Node *np1, Node *np2),
	*castcode(Node *child, Type *tp),
	*sizeofcode(Type *tp), 
	*ternarycode(Node *cond, Node *ifyes, Node *ifno),
	*constcode(Symbol *sym);

#define SYM(s) ((union unode) {.sym = s})
#define OP(s) ((union unode) {.op = s})
#define TYP(s) ((union unode) {.type = s})
#define ISNODESYM(n) ((n)->code == emitsym)
#define ISNODEBIN(n) ((n)->code == emitbin)
#define ISNODELOG(n) (ISNODEBIN(n) && (n)->u.op & 0x40)

#endif
