
typedef struct symbol Symbol;
typedef struct node Node;

typedef struct {
	short size;
	uint8_t align;
	char letter;
	bool sign : 1;
	bool c_int : 1;
} Type;

struct symbol {
	char *name;
	bool public : 1;
	bool extrn : 1;
	char type;
	union {
		struct {
			Type *type;
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
	Type *type;
	uint8_t complex;
	uint8_t addable;
	union {
		Symbol *sym;
		TINT imm;
		char reg;
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

#define FUN        0
#define VAR        1
#define EFUN       2
#define AUTO      'A'
#define REG       'R'
#define MEM       'T'
#define PAR       'P'
#define CONST     '#'
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


enum {
	PUSH, POP, LD, ADD, RET, ADDI, LDI, ADDX, ADCX, LDX,
	LDFX
};

enum {
	A = 1, B, C, D, E, H, L, IYL, IYH, NREGS,
	IXL, IXH, F, I, SP, AF, HL, DE, BC, IX, IY
};

extern void error(unsigned nerror, ...);
extern Node *genaddable(Node *np);
extern void generate(Symbol *fun);
extern void genstack(Symbol *fun);
extern void apply(Node *list[], Node *(*fun)(Node *));
extern Symbol *parse(void);
extern void code(char op, ...);
extern Node *optimize(Node *np);
extern void prtree(Node *np);
