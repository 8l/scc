/* See LICENSE file for copyright and license details. */

enum iflags {
	BBENTRY =    1,        /* basic block entry */
};

enum tflags {
	SIGNF   =     1 << 0,  /* Signed type */
	INTF    =     1 << 1,  /* integer type */
	FLOATF  =     1 << 2,  /* float type */
	STRF    =     1 << 3,  /* string */
	AGGRF   =     1 << 4,  /* aggregate */
	FUNF    =     1 << 5,  /* function */
	PARF    =     1 << 6,  /* parameter */
	INITF   =     1 << 7,  /* initializer flag */
	ELLIPS  =     1 << 8,  /* vararg function */
};

enum sclass {
	SAUTO     = 'A',
	SREG      = 'R',
	SLABEL    = 'L',
	SINDEX    = 'I',
	STMP      = 'N',
	SGLOB     = 'G',
	SEXTRN    = 'X',
	SPRIV     = 'Y',
	SLOCAL    = 'T',
	SMEMB     = 'M',
	SCONST    = '#',
	STRING    = '"',
	SNONE     = 0 /* cc2 relies on SNONE being 0 in nextpc() */
};

enum types {
	ELLIPSIS = 'E',
	INT8     = 'C',
	INT16    = 'I',
	INT32    = 'W',
	INT64    = 'Q',
	UINT8    = 'K',
	UINT16   = 'N',
	UINT32   = 'Z',
	UINT64   = 'O',
	POINTER  = 'P',
	FUNCTION = 'F',
	VECTOR   = 'V',
	UNION    = 'U',
	STRUCT   = 'S',
	BOOL     = 'B',
	FLOAT    = 'J',
	DOUBLE   = 'D',
	LDOUBLE  = 'H',
	VOID     = '0'
};

enum op {
	/* kind of operand */
	/* operands */
	OMEM     = 'M',
	OTMP     = 'N',
	OAUTO    = 'A',
	OREG     = 'R',
	OCONST   = '#',
	OSTRING  = '"',
	OLOAD    = 'D',
	OLABEL   = 'L',
	OADD     = '+',
	OSUB     = '-',
	OMUL     = '*',
	OMOD     = '%',
	ODIV     = '/',
	OSHL     = 'l',
	OSHR     = 'r',
	OLT      = '<',
	OGT      = '>',
	OLE      = '[',
	OGE      = ']',
	OEQ      = '=',
	ONE      = '!',
	OBAND    = '&',
	OBOR     = '|',
	OBXOR    = '^',
	OCPL     = '~',
	OASSIG   = ':',
	OSNEG    = '_',
	OCALL    = 'c',
	OCALLE   = 'z',
	OPAR     = 'p',
	OFIELD   = '.',
	OCOMMA   = ',',
	OASK     = '?',
	OCOLON   = ' ',
	OADDR    = '\'',
	OAND     = 'a',
	OOR      = 'o',
	ONEG     = 'n',
	OPTR     = '@',
	OCAST    = 'g',
	OINC     = 'i',
	ODEC     = 'd',
	OBUILTIN = 'm',
	/*statements */
	ONOP     = 'q',
	OJMP     = 'j',
	OBRANCH  = 'y',
	ORET     = 'h',
	OBLOOP   = 'b',
	OELOOP   = 'e',
	OCASE    = 'v',
	ODEFAULT = 'f',
	OBSWITCH = 's',
	OESWITCH = 't',
	OBFUN    = 'x',
	OEFUN    = 'k',
};

enum builtins {
	BVA_START = 's',
	BVA_END   = 'e',
	BVA_ARG   = 'a',
	BVA_COPY  = 'c',
};

enum nerrors {
	EEOFFUN,       /* EOF while parsing function */
	ENLABEL,       /* label without statement */
	EIDOVER,       /* identifier overflow */
	EOUTPAR,       /* out pf params */
	ESYNTAX,       /* syntax error */
	ESTACKA,       /* stack unaligned */
	ESTACKO,       /* stack overflow */
	ESTACKU,       /* stack underflow */
	ELNLINE,       /* line too long */
	ELNBLNE,       /* line without new line */
	EFERROR,       /* error reading from file:%s*/
	EBADID,        /* incorrect symbol id */
	EWTACKO,       /* switch stack overflow */
	EWTACKU,       /* switch stack underflow */
	ENOSWTC,       /* Out of switch statement */
	EBBUILT,       /* Unknown builtin */
	ENUMERR
};

typedef struct node Node;
typedef struct type Type;
typedef struct symbol Symbol;
typedef struct addr Addr;
typedef struct inst Inst;

struct type {
	unsigned long size;
	unsigned long align;
	short flags;
};

struct symbol {
	Type type;
	Type rtype;
	unsigned short id;
	unsigned short numid;
	char *name;
	char kind;
	union {
		unsigned long off;
		Node *stmt;
		Inst *inst;
	} u;
	Symbol *next;
	Symbol *h_next;
};

struct node {
	char op;
	Type type;
	char complex;
	char address;
	unsigned char flags;
	union {
		TUINT i;
		TFLOAT f;
		char reg;
		char *s;
		Symbol *sym;
		char subop;
	} u;
	Symbol *label;
	Node *left, *right;
	Node *next, *prev;
};

struct addr {
        char kind;
        union {
                char reg;
                TUINT i;
                Symbol *sym;
        } u;
};

struct inst {
	unsigned char op;
	unsigned char flags;
	Symbol *label;
	Inst *next, *prev;
	Addr from1, from2, to;
};

/* main.c */
extern void error(unsigned nerror, ...);

/* parse.c */
extern void parse(void);

/* optm.c */
extern Node *optm_dep(Node *np), *optm_ind(Node *np);

/* cgen.c */
extern Node *sethi(Node *np);
extern Node *cgen(Node *np);

/* peep.c */
extern void peephole(void);

/* code.c */
extern void data(Node *np);
extern void writeout(void), endinit(void), newfun(void);
extern void code(int op, Node *to, Node *from1, Node *from2);
extern void defvar(Symbol *), defpar(Symbol *), defglobal(Symbol *);
extern void setlabel(Symbol *sym), getbblocks(void);
extern Node *label2node(Node *np, Symbol *sym);
extern Node *constnode(Node *np, TUINT n, Type *tp);
extern Symbol *newlabel(void);

/* node.c */
#define SETCUR  1
#define KEEPCUR 0
extern void apply(Node *(*fun)(Node *));
extern void cleannodes(void);
extern void delnode(Node *np);
extern void deltree(Node *np);
extern void prtree(Node *np), prforest(char *msg);
extern Node *newnode(int op);
extern Node *addstmt(Node *np, int flags);
extern Node *delstmt(void);
extern Node *nextstmt(void);

/* symbol.c */
#define TMPSYM  0
extern Symbol *getsym(unsigned id);
extern void popctx(void);
extern void pushctx(void);
extern void freesym(Symbol *sym);

/* globals */
extern Symbol *curfun;
extern Symbol *locals;
extern Inst *pc, *prog;
