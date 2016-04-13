
enum tflags {
	SIGNF   =    1,
	INTF    =    2,
	STRF    =    8,
	UNIONF  =    16,
	FUNF    =    32,
	INITF   =   128
};

enum op {
	/* types */
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
	VOID     = '0',
	ONAME    = '"',
	/* kind of operand */
	NONE     = 0,
	MEM      = 'M',
	AUTO     = 'A',
	REG      = 'R',
	CONST    = '#',
	STRING   = '"',
	LABEL    = 'L',
	INDEX    = 'I',
	/* storage class */
	GLOB     = 'G',
	EXTRN    = 'X',
	PRIVAT   = 'Y',
	LOCAL    = 'T',
	MEMBER   = 'M',
	/* operands */
	OMEM     = 'M',
	OAUTO    = 'A',
	OREG     = 'R',
	OCONST   = '#',
	OSTRING  = '"',
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
	ONEG     = '_',
	OCALL    = 'c',
	OPAR     = 'p',
	OFIELD   = '.',
	OCOMMA   = ',',
	OASK     = '?',
	OCOLON   = ' ',
	OADDR    = '\'',
	OAND     = 'a',
	OOR      = 'o',
	OPTR     = '@',
	OSYM     = 'i',
	OCAST    = 'g',
	OINC     = 'i',
	ODEC     = 'd',
	/*statements */
	ONOP     = 'n',
	OJMP     = 'j',
	OBRANCH  = 'y',
	ORET     = 'h',
	OBLOOP   = 'b',
	OELOOP   = 'e',
	OCASE    = 'v',
	ODEFAULT = 'f',
	OTABLE   = 't',
	OSWITCH  = 's',
	OEPARS   = '\\',
	OSTMT    = '\t'
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
	EFERROR,       /* error reading from file:%s*/
	ENUMERR
};

typedef struct node Node;
typedef struct type Type;
typedef struct symbol Symbol;
typedef struct addr Addr;
typedef struct inst Inst;

struct type {
	TSIZE size;
	TSIZE align;
	char flags;
};

struct symbol {
	Type type;
	unsigned short id;
	unsigned short numid;
	char *name;
	char kind;
	union {
		TSIZE off;
		Node *label;
	} u;
	Symbol *next;
	Symbol *h_next;
};

struct node {
	char op;
	Type type;
	char complex;
	char address;
	union {
		TUINT i;
		char reg;
		char *s;
		Symbol *sym;
		char subop;
	} u;
	Symbol *label;
	Node *left, *right;
	Node *stmt;
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
        char op;
        Addr from1, from2, to;
        Inst *next, *prev;
};

/* main.c */
extern void error(unsigned nerror, ...);

/* parse.c */
extern void parse(void);

/* optm.c */
extern void optimize(void);

/* cgen.c */
extern void addressability(void);
extern void generate(void);

/* peep.c */
extern void peephole(void);

/* code.c */
extern void data(Node *np);
extern void writeout(void), endinit(void), newfun(void);
extern void defvar(Symbol *), defpar(Symbol *), defglobal(Symbol *);

/* node.c */
extern void cleannodes(void);
extern void delnode(Node *np);
extern void deltree(Node *np);
extern Node *newnode(void);
extern Symbol *curfun;

/* symbol.c */
extern Symbol *getsym(int id);
extern void popctx(void);
extern void pushctx(void);
extern void freesym(Symbol *sym);
