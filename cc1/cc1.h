

#define INPUTSIZ LINESIZ

#define GLOBALCTX 0


/*
 * Definition of structures
 */
typedef struct type Type;
typedef struct symbol Symbol;
typedef struct swtch Switch;
typedef struct node Node;
typedef struct input Input;

struct limits {
	union {
		TUINT i;
		TFLOAT f;
	} max;
	union {
		TUINT i;
		TFLOAT f;
	} min;
};

struct keyword {
	char *str;
	unsigned char token, value;
};

struct type {
	unsigned char op;           /* type builder operator */
	char ns;                    /* namespace for struct members */
	short id;                   /* type id, used in dcls */
	char letter;                /* letter of the type */
	bool defined : 1;           /* type defined */
	bool sign : 1;              /* signess of the type */
	bool printed : 1;           /* the type already was printed */
	bool integer : 1;           /* this type is INT or enum */
	bool arith : 1;             /* this type is INT, ENUM, FLOAT */
	bool aggreg : 1;            /* this type is struct or union */
	bool k_r : 1;               /* This is a k&r function */
	TSIZE size;                 /* sizeof the type */
	TSIZE align;                /* align of the type */
	Type *type;                 /* base type */
	Symbol *tag;                /* symbol of the strug tag */
	Type *next;                 /* next element in the hash */
	union {
		Type **pars;            /* Function type parameters */
		Symbol **fields;        /* fields of aggregate type */
	} p;
	union {
		unsigned char rank;     /* convertion rank */
		TINT elem;              /* number of type parameters */
	} n;
};

struct symbol {
	char *name;
	Type *type;
	unsigned short id;
	unsigned char ctx;
	char ns;
	unsigned char token;
	short flags;
	union {
		TINT i;
		TUINT u;
		TFLOAT f;
		char *s;
		unsigned char token;
		Node **init;
		Symbol **pars;
	} u;
	struct symbol *next;
	struct symbol *hash;
};

struct node {
	unsigned char op;
	Type *type;
	Symbol *sym;
	char flags;
	struct node *left, *right;
};

struct swtch {
	short nr;
	char hasdef;
};

struct yystype {
	Symbol *sym;
	unsigned char token;
};

struct input {
	char *fname;
	FILE *fp;
	char *line, *begin, *p;
	struct input *next;
	unsigned short nline;
};

/*
 * Definition of enumerations
 */

/* data type letters */
enum {
	L_INT8      = 'C',
	L_INT16     = 'I',
	L_INT32     = 'W',
	L_INT64     = 'Q',
	L_UINT8     = 'K',
	L_UINT16    = 'N',
	L_UINT32    = 'Z',
	L_UINT64    = 'O',
	L_BOOL      = 'B',

	L_FLOAT     = 'J',
	L_DOUBLE    = 'D',
	L_LDOUBLE   = 'H',

	L_ELLIPSIS  = 'E',
	L_VOID      = '0',
	L_POINTER   = 'P',
	L_FUNCTION  = 'F',
	L_ARRAY     = 'V',
	L_UNION     = 'U',
	L_STRUCT    = 'S',
};

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
	KRFTN
};

/* namespaces */
enum {
	NS_IDEN = 1,
	NS_TAG,
	NS_LABEL,
	NS_CPP,
	NS_KEYWORD,
	NS_CPPCLAUSES,
	NS_STRUCTS
};

/* symbol flags */
enum {
	SAUTO     = 1 << 0,
	SREGISTER = 1 << 1,
	SDECLARED = 1 << 2,
	SFIELD    = 1 << 3,
	SEXTERN   = 1 << 4,
	SUSED     = 1 << 5,
	SCONSTANT = 1 << 6,
	SGLOBAL   = 1 << 7,
	SPRIVATE  = 1 << 8,
	SLOCAL    = 1 << 9,
	SEMITTED  = 1 << 10,
	SDEFINED  = 1 << 11,
	SSTRING   = 1 << 12,
	STYPEDEF  = 1 << 13,
	SINITLST  = 1 << 14,
	SHASINIT  = 1 << 15
};

/* node flags */
enum {
	NLVAL   = 1 << 0,
	NCONST  = 1 << 1,
	NEFFECT = 1 << 2
};

/* lexer mode, compiler or preprocessor directive */
enum {
	CCMODE,
	CPPMODE
};

/* input tokens */
enum tokens {
	CONST      = 1 << 0,      /* type qualifier tokens are used as flags */
	RESTRICT   = 1 << 1,
	VOLATILE   = 1 << 2,
	INLINE     = 1 << 3,
	TQUALIFIER = 1 << 7,      /* this value is picked outside of ASCII range */
	TYPE,
	IDEN,
	SCLASS,
	CONSTANT,
	STRING,
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
	DEFINE,
	INCLUDE,
	LINE,
	PRAGMA,
	ERROR,
	IFDEF,
	ELIF,
	IFNDEF,
	UNDEF,
	ENDIF,
	EOFTOK
};

/* operations */
enum op {
	OADD,
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
	ONEG,
	OCPL,
	OAND,
	OOR,
	OEQ,
	ONE,
	OLT,
	OGE,
	OLE,
	OGT,
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
	OADDR,
	OCOMMA,
	OCAST,
	OPTR,
	OSYM,
	OASK,
	OCOLON,
	OFIELD,
	OLABEL,
	ODEFAULT,
	OCASE,
	OJUMP,
	OBRANCH,
	OEXPR,
	OEFUN,
	OELOOP,
	OBLOOP,
	OFUN,
	OPAR,
	OCALL,
	ORET,
	ODECL,
	OBSWITCH,
	OESWITCH,
	OINIT
};

/* error.c */
extern void error(char *fmt, ...);
extern void warn(char *fmt, ...);
extern void unexpected(void);
extern void errorp(char *fmt, ...);
extern void cpperror(char *fmt, ...);

/* types.c */
extern bool eqtype(Type *tp1, Type *tp2);
extern Type *ctype(unsigned type, unsigned sign, unsigned size);
extern Type *mktype(Type *tp, int op, TINT nelem, Type *data[]);
extern Type *duptype(Type *base);
extern struct limits *getlimits(Type *tp);
extern void typesize(Type *tp);

/* symbol.c */
extern void dumpstab(char *msg);
extern Symbol *lookup(int ns, char *name);
extern Symbol *nextsym(Symbol *sym, int ns);
extern Symbol *install(int ns, Symbol *sym);
extern Symbol *newsym(int ns);
extern void pushctx(void), popctx(void);
extern void killsym(Symbol *sym);
extern Symbol *newlabel(void);
extern void keywords(struct keyword *key, int ns);
extern Symbol *newstring(char *s, size_t len);

/* stmt.c */
extern void compound(Symbol *lbreak, Symbol *lcont, Switch *sw);

/* decl.c */
extern Type *typename(void);
extern void decl(void);

/* lex.c */
extern char ahead(void);
extern unsigned next(void);
extern bool moreinput(void);
extern void expect(unsigned tok);
extern void discard(void);
extern bool addinput(char *fname);
extern void setsafe(int type);
extern void ilex(char *fname);
#define accept(t) ((yytoken == (t)) ? next() : 0)

/* code.c */
extern void emit(unsigned, void *);
extern Node *node(unsigned op, Type *tp, Node *left, Node *rigth);
extern Node *varnode(Symbol *sym);
extern Node *constnode(Symbol *sym);
extern Node *sizeofnode(Type *tp);
extern void freetree(Node *np);
#define BTYPE(np) ((np)->type->op)

/* fold.c */
extern Node *simplify(int op, Type *tp, Node *lp, Node *rp);
extern Node *castcode(Node *np, Type *newtp);
extern TUINT ones(int nbytes);

/* expr.c */
extern Node *decay(Node *), *negate(Node *np), *assign(void);;
extern Node *convert(Node *np, Type *tp1, char iscast);
extern Node *iconstexpr(void), *condexpr(void), *expr(void);
extern bool isnodecmp(int op);
extern int negop(int op);
extern bool cmpnode(Node *np, TUINT val);

/* init.c */
extern void initializer(Symbol *sym, Type *tp);
extern Node *initlist(Type *tp);

/* cpp.c */
extern void icpp(void);
extern bool cpp(void);
extern bool expand(char *begin, Symbol *sym);
extern void incdir(char *dir);
extern void outcpp(void);
extern Symbol *defmacro(char *s);

/*
 * Definition of global variables
 */
extern struct yystype yylval;
extern char yytext[];
extern unsigned yytoken;
extern unsigned short yylen;
extern int cppoff, disexpand;
extern unsigned cppctx;
extern Input *input;
extern int lexmode, namespace, onlycpp;
extern unsigned curctx;
extern Symbol *curfun, *zero, *one;

extern Type *voidtype, *pvoidtype, *booltype,
            *uchartype,   *chartype, *schartype,
            *uinttype,    *inttype,
            *sizettype, *pdifftype,
            *ushortype,   *shortype,
            *longtype,    *ulongtype,
            *ullongtype,  *llongtype,
            *floattype,   *doubletype,  *ldoubletype,
            *ellipsistype;
