#ifndef CC1_H
#define CC1_H



struct user_opt {
	unsigned char implicit;
	unsigned char mixdcls;
	unsigned char npromote;
	unsigned char useless;
	unsigned char charsign;
	unsigned char pcompare;
};

extern struct user_opt options;
extern void error(const char *fmt, ...);
extern void warn(signed char flag, const char *fmt, ...);
extern void unexpected(void);

/* definitions of types */

#define CTX_OUTER 0
#define CTX_FUNC  1

enum {
	NS_IDEN = 0,
	NS_TAG,
	NS_LABEL,
	NS_STRUCTS,
	NR_NAMESPACES
};

typedef struct ctype Type;
typedef struct symbol Symbol;

struct ctype {
	uint8_t op;           /* type builder operator */
	uint8_t ns;
	char letter;          /* letter of the type */
	bool defined : 1;     /* type defined (is not a forward reference) */
	bool sign : 1;        /* sign type */
	struct ctype *type;   /* base type */
	struct ctype *next;   /* next element in the hash */
	Type **pars;         /* type parameters */
	union {
		unsigned char rank;  /* convertion rank */
		short elem;          /* number of type parameters */
	} n;
};


/* definition of symbols */

struct symbol {
	char *name;
	Type *type;
	short id;
	uint8_t ctx;
	uint8_t ns;
	uint8_t token;
	bool isglobal : 1;
	bool isstatic : 1;
	bool isauto : 1;
	bool isregister : 1;
	bool isdefined : 1;
	bool isfield : 1;
	bool isparameter : 1;
	union {
		int i;
		char *s;
		uint8_t token;
	} u;
	struct symbol *next;
	struct symbol *hash;
};

extern bool eqtype(Type *tp1, Type *tp2);
extern Type *ctype(int8_t type, int8_t sign, int8_t size),
	*mktype(Type *tp, uint8_t op, short nelem, void *data);

extern Symbol
	*lookup(char *s, unsigned char ns),
	*install(char *s, unsigned char ns);

extern void pushctx(void), popctx(void);

typedef struct caselist Caselist;

extern void compound(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

extern Type *typename(void);

extern Type *voidtype, *pvoidtype, *booltype,
	*uchartype,   *chartype,
	*uinttype,    *inttype,
	*ushortype,   *shortype,
	*longtype,    *ulongtype,
	*ullongtype,  *llongtype,
	*floattype,   *doubletype,  *ldoubletype;

enum {
	FTN = 1, ENUM, TYPEIDEN, VOID, FLOAT, INT, BOOL,
	STRUCT, UNION, PTR, ARY, CHAR, DOUBLE, SHORT,
	LONG, COMPLEX, UNSIGNED, SIGNED
};

#define CONST         (1<<0)
#define VOLATILE      (1<<1)
#define RESTRICT      (1<<2)

#define TYPEDEF       1
#define EXTERN        2
#define STATIC        3
#define AUTO          4
#define REGISTER      5

#define accept(t) ((yytoken == (t)) ? next() : 0)
extern uint8_t ahead(void);

enum tokens {
	TQUALIFIER = 128, TYPE, IDEN, SCLASS,
	CONSTANT, SIZEOF,
	INDIR, INC, DEC, SHL, SHR,
	LE, GE, EQ, NE, AND, OR,
	MUL_EQ, DIV_EQ, MOD_EQ, ADD_EQ, SUB_EQ, AND_EQ,
	XOR_EQ, OR_EQ, SHL_EQ, SHR_EQ,
	ELLIPSIS, STRING,
	CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO,
	CONTINUE, BREAK, RETURN, EOFTOK, NOTOK
};

struct yystype {
	Symbol *sym;
	uint8_t token;
};

extern struct yystype yylval;
extern char yytext[];
extern uint8_t yytoken;

extern uint8_t next(void);
extern void expect(uint8_t tok);


typedef struct node {
	void (*code)(struct node *);
	Type *type;
	uint8_t typeop;
	uint8_t nchilds;
	struct {
		bool lvalue : 1;
		bool symbol: 1;
		bool constant : 1;
	} b;
	union unode {
		Symbol *sym;
		Type *type;
		char op;
	} u;
	struct node *childs[];
} Node;

enum {
	OCAST = 1, OPTR, OADD, OSIZE, OMUL, OSUB,
	OINC, ODEC, ODIV, OMOD, OSHL, OSHR,
	OBAND, OBXOR, OBOR, OASSIGN, OA_MUL, OA_DIV,
	OA_MOD, OA_ADD, OA_SUB, OA_SHL, OA_SHR,
	OA_AND, OA_XOR, OA_OR, OADDR,ONEG, OCPL, OEXC,
	OCOMMA,
	/* TODO: This order is important, but must be changed */
	OAND, OOR,
	/*
	  * Complementary relational operators only differ in less
	 * significant bit
	 */
	OEQ = 0x40, ONE, OLT, OGE, OLE, OGT
};

extern void
	emitdcl(Symbol *), emitefun(void),
	emitsym(Node *), emitunary(Node *),
	emitbin(Node *), emitexp(Node *),
	emitprint(Node *), emitlabel(Symbol *), emitjump(Symbol *, Node *),
	emitbloop(void), emiteloop(void),
	emitswitch(short, Node *), emitcase(Symbol *, Node *),
	emitret(Type *tp),
	emitfun(Symbol *sym),
	emitdefault(Symbol *),
	emitstruct(Symbol *sym), emitestruct(void);

extern Node
	*node(void (*code)(Node *),
	      Type *tp, union unode u, uint8_t nchilds),
	*unarycode(char op, Type *tp, Node *child),
	*bincode(char op, Type *tp, Node *np1, Node *np2),
	*castcode(Node *child, Type *tp),
	*sizeofcode(Type *tp), 
	*ternarycode(Node *cond, Node *ifyes, Node *ifno),
	*symcode(Symbol *sym),
	*fieldcode(Node *child, Symbol *field);

extern void freetree(Node *np);

#define NEGATE(n, v) ((n)->u.op ^= (v))
/* TODO: remove some of these ugly macros */
#define ISNODEBIN(n) ((n)->code == emitbin)
#define ISNODECMP(n) (ISNODEBIN(n) && (n)->u.op >= OEQ)
#define ISNODELOG(n) (ISNODEBIN(n) && (n)->u.op >= OAND)

extern Node *expr(void);
extern void extdecl(void), decl(void);

#endif
