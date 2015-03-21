
#define SIGNF 1
#define INTF  2

#define NONE       0
#define AUTO      'A'
#define REG       'R'
#define MEM       'T'
#define PAR       'P'
#define CONST     '#'
#define INDEX     'I'
#define LABEL     'L'
#define OADD      '+'
#define OSUB      '-'
#define OASSIG    ':'
#define OINC      ';'
#define OMOD      '%'
#define ODIV      '/'
#define OSHL      'l'
#define OSHR      'r'
#define OBAND     '&'
#define OBOR      '|'
#define OBXOR     '^'
#define OPTR      '@'
#define OADDR     'a'
#define OLT       '<'
#define OGT       '>'
#define OGE       ']'
#define OLE       '['
#define OEQ       '='
#define ONE       '!'
#define OOR       'o'
#define OAND      'y'
#define OCAST     'c'
#define ONEG      '_'
#define OCPL      '~'
#define OCOMMA    ','
#define ORET      'y'

#define ADDABLE 10


typedef struct symbol Symbol;
typedef struct node Node;
typedef struct inst Inst;
typedef struct addr Addr;
typedef struct type Type;

struct type {
	unsigned short size;
	uint8_t align;
	char letter;
	uint8_t flags;
};

struct symbol {
	unsigned short id;
	char *name;
	char kind;
	bool public : 1;
	bool extrn : 1;
	bool index : 1;
	union {
		/* TODO: Admit inmediate of other type */
		TINT imm;
		struct {
			Type type;
			char sclass;
			short off;
		} v;
		Inst *pc;
		struct {
			short locals;
			short params;
			Node **body;
		} f;
	} u;
};

struct node {
	uint8_t op;
	uint8_t subop;
	Type type;
	uint8_t complex;
	uint8_t addable;
	uint8_t reg;
	Symbol *sym;
	bool used : 1;
	struct node *left, *right;
};


struct addr {
	char kind;
	union {
		uint8_t reg;
		TINT i;
		Symbol *sym;
	} u;
};

struct inst {
	uint8_t op;
	Addr from, to;
	Symbol *label;
	Inst *next, *prev;
};

enum nerrors {
	EINTNUM,       /* too much internal identifiers */
	EEXTNUM,       /* too much external identifiers */
	EPARNUM,       /* too much parameters */
	ENODEOV,       /* node overflow */
	ESTACKO,       /* stack overflow */
	ESTACKU,       /* stack underflow */
	EEXPROV,       /* expression overflow */
	ETYPERR,       /* incorrect type in expression */
	EEXPBAL,       /* expression not balanced */
	ESYNTAX,       /* syntax error */
	ELNLINE,       /* line too long */
	EFERROR,       /* error reading from file:%s*/
	ENUMERR
};

enum {
	LDW,
	LDL,
	LDH,
	MOV,
	LDI,
	ADD,
	PUSH,
	POP,
	RET,
	NOP,
	INC,
	SUB,
	DEC,
	JP,
	AND,
	OR,
	XOR,
	CPL,
	NEG
};

enum {
	A = 1, B, C, D, E, H, L, IYL, IYH, NREGS,
	AF = NREGS, HL, DE, BC, IY, NPAIRS,
	SP = NPAIRS, IX
};

extern Symbol *curfun;
extern Inst *prog, *pc;

/* main.c */
extern void error(unsigned nerror, ...);

/* cgen.c */
extern void addable(void);
extern void generate(void);
extern void apply(Node *(*fun)(Node *));

/* parser.c */
extern void parse(void);
extern void prtree(Node *np);

/* code.c */
extern void code(uint8_t op, Node *to, Node *from);
extern void inscode(uint8_t op, Addr *to, Addr *from);
extern void writeout(void);
extern void delcode(void);

/* optm.c */
extern void optimize(void);
extern Node *imm(TINT i);

/* peep.c */
extern void peephole(void);
