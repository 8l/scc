

/*
 * Definition of structures
 */
typedef struct type Type;
typedef struct symbol Symbol;
typedef struct caselist Caselist;
typedef struct node Node;

struct type {
	uint8_t op;           /* type builder operator */
	uint8_t ns;
	char letter;          /* letter of the type */
	bool defined;         /* type defined (is not a forward reference) */
	Type *type;           /* base type */
	Type *next;           /* next element in the hash */
	Type **pars;          /* type parameters */
	union {
		unsigned char rank;  /* convertion rank */
		short elem;          /* number of type parameters */
	} n;
};

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

struct node {
	uint8_t op;
	Type *type;
	Symbol *sym;
	bool lvalue : 1;
	bool symbol: 1;
	bool constant : 1;
	struct node *left, *right;
};

struct scase {
	Symbol *label;
	Node *expr;
	struct scase *next;
};

struct caselist {
	short nr;
	Symbol *deflabel;
	struct scase *head;
};

struct yystype {
	Symbol *sym;
	uint8_t token;
};

/*
 * Definition of enumerations
 */

/* recovery points */
enum {
	END_DECL,
	END_LDECL,
	END_COMP,
	END_COND
};

/* type constructors */
enum {
	FTN = 1,
	PTR,
	ARY,
};

/* namespaces */
enum {
	NS_IDEN,
	NS_TAG,
	NS_LABEL,
	NS_CPP,
	NS_STRUCTS,
	NR_NAMESPACES
};

/* input tokens */
enum tokens {
	TQUALIFIER = 128,
	TYPE,
	IDEN,
	SCLASS,
	CONSTANT,
	SIZEOF,
	INDIR,
	INC,
	DEC,
	SHL,
	SHR,
	LE,
	GE,
	EQ,
	NE,
	AND,
	OR,
	MUL_EQ,
	DIV_EQ,
	MOD_EQ,
	ADD_EQ,
	SUB_EQ,
	AND_EQ,
	XOR_EQ,
	OR_EQ,
	SHL_EQ,
	SHR_EQ,
	ELLIPSIS,
	CASE,
	DEFAULT,
	IF,
	ELSE,
	SWITCH,
	WHILE,
	DO,
	FOR,
	GOTO,
	VOID,
	FLOAT,
	INT,
	BOOL,
	STRUCT,
	UNION,
	CHAR,
	DOUBLE,
	SHORT,
	LONG,
	LLONG,
	COMPLEX,
	CONST,
	VOLATILE,
	RESTRICT,
	TYPEDEF,
	EXTERN,
	STATIC,
	AUTO,
	REGISTER,
	ENUM,
	TYPEIDEN,
	UNSIGNED,
	SIGNED,
	CONTINUE,
	BREAK,
	RETURN,
	EOFTOK,
	NOTOK
};

/* operations */
enum {
	OPTR,
	OADD,
	OSIZE,
	OMUL,
	OSUB,
	OINC,
	ODEC,
	ODIV,
	OMOD,
	OSHL,
	OSHR,
	OBAND,
	OBXOR,
	OBOR,
	OASSIGN,
	OA_MUL,
	OA_DIV,
	OA_MOD,
	OA_ADD,
	OA_SUB,
	OA_SHL,
	OA_SHR,
	OA_AND,
	OA_XOR,
	OA_OR,
	OADDR,ONEG,
	OCPL,
	OEXC,
	OCOMMA,
	OCAST,
	OSYM,
	OASK,
	OCOLON,
	OFIELD,
	OTYP,
	OLABEL,
	ODEFAULT,
	OCASE,
	OSTRUCT,
	OJUMP,
	OBRANCH,
	OEXPR,
	OEFUN,
	OESTRUCT,
	OELOOP,
	OBLOOP,
	OPRINT,
	OFUN,
	ORET,
	ODECL,
	OSWITCH,
	OAND,
	OOR,
	OEQ,
	ONE,
	OLT,
	OGE,
	OLE,
	OGT
};

/* error.c */
extern void error(char *fmt, ...);
extern void warn(char *fmt, ...);
extern void unexpected(void);
extern void softerror(char *fmt, ...);
extern void setsafe(uint8_t type);

/* type.c */
extern bool eqtype(Type *tp1, Type *tp2);
extern Type *ctype(uint8_t type, uint8_t sign, uint8_t size);
extern Type *mktype(Type *tp, uint8_t op, short nelem, void *data);

/* symbol.c */
extern Symbol *lookup(char *s, unsigned char ns);
extern Symbol *install(char *s, unsigned char ns);
extern void pushctx(void), popctx(void);

/* stmt.c */
extern void compound(Symbol *lbreak, Symbol *lcont, Caselist *lswitch);

/* decl.c */
extern Type *typename(void);
extern void extdecl(void), decl(void);

/* lex.c */
extern uint8_t ahead(void);
extern uint8_t next(void);
extern void expect(uint8_t tok);
extern void discard(void);
extern char *filename(void);
extern unsigned short fileline(void);
extern bool addinput(char *fname);
extern void delinput(void);
#define accept(t) ((yytoken == (t)) ? next() : 0)

/* code.c */
extern void emit(uint8_t, void *);
extern Node *node(uint8_t op, Type *tp, Node *left, Node *rigth);
extern Node *symbol(Symbol *sym);
extern void freetree(Node *np);

/* expr.c */
extern Node *expr(void), *negate(Node *np);

/* cpp.c */
extern char *preprocessor(char *s);

/*
 * Definition of global variables
 */
extern struct yystype yylval;
extern char yytext[];
extern uint8_t yytoken;

extern Type *voidtype, *pvoidtype, *booltype,	
            *uchartype,   *chartype,
            *uinttype,    *inttype,
            *ushortype,   *shortype,
            *longtype,    *ulongtype,
            *ullongtype,  *llongtype,
            *floattype,   *doubletype,  *ldoubletype;
