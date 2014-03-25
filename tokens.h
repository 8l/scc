#ifndef TOKENS_H
#define TOKENS_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

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

#define accept(t) ((bool) (yytoken == (t) ? next() : 0))
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

struct symbol;
union yystype {
	struct symbol *sym;
	uint8_t token;
};

extern union yystype yylval;
extern char yytext[];
extern uint8_t yytoken, yyntoken;


extern uint8_t next(void);
extern void expect(uint8_t tok);

#endif
