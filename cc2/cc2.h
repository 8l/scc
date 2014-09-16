
typedef struct {
	short size;
	uint8_t align;
	bool sign : 1;
	bool c_int : 1;
} Type;

typedef struct symbol {
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
		} f;
	} u;
} Symbol;

typedef struct node {
	char op;
	char subop;
	Type *type;
	uint8_t complex;
	uint8_t addable;
	union {
		Symbol *sym;
		int imm;
	} u;
	struct node *left, *right;
} Node;

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
#define ONEG      '_'
#define OCPL      '~'
#define OCOMMA    ','

extern void error(unsigned nerror, ...);
extern void genaddable(Node *list[]);
extern void generate(Symbol *sym, Node *list[]);
extern void genstack(Symbol *fun);
