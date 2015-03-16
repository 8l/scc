
#define SIGNF 1
#define INTF  2

#define NONE       0
#define FUN        0
#define VAR        1
#define EFUN       2
#define AUTO      'A'
#define REG       'R'
#define MEM       'T'
#define PAR       'P'
#define CONST     '#'
#define PUSHED    'S'
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

#define ADDABLE 10


typedef struct symbol Symbol;
typedef struct node Node;

typedef struct {
	short size;
	uint8_t align;
	char letter;
	uint8_t flags;
} Type;

struct symbol {
	char *name;
	bool public : 1;
	bool extrn : 1;
	char type;
	unsigned short id;
	uint8_t reg;
	union {
		struct {
			Type type;
			char sclass;
			short off;
		} v;
		struct {
			short addr;
		} l;
		struct {
			short locals;
			short params;
			Node **body;
		} f;
	} u;
};

struct node {
	char op;
	char subop;
	Type type;
	uint8_t complex;
	uint8_t addable;
	uint8_t kind;
	union {
		Symbol *sym;
		/* TODO: Admit inmediate of other type */
		TINT imm;
		uint8_t reg;
	} u;
	struct node *left, *right;
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
	INC
};

enum {
	A = 1, B, C, D, E, H, L, IYL, IYH, NREGS,
	SP = NREGS, AF, HL, DE, BC, IX, IY
};

extern Type Funct, l_int8,  l_int16,  l_int32,  l_int64,
                   l_uint8, l_uint16, l_uint32, l_uint64;

extern Symbol *curfun;

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
extern void writeout(void);

/* optm.c */
extern void optimize(void);
extern Node *imm(TINT i, Type *tp);
