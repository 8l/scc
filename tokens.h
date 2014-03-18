#ifndef TOKENS_H
#define TOKENS_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

#define FLOAT         1
#define INT           2
#define BOOL          3
#define PTR           4
#define ARY           5
#define FTN           6

#define VOID          7
#define STRUCT        8
#define UNION         9
#define ENUM         10
#define TYPENAME     11

#define CHAR         12
#define DOUBLE       13
#define SHORT        14
#define LONG         15

#define COMPLEX      16
#define IMAGINARY    17
#define UNSIGNED     18
#define SIGNED       19

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


extern void init_lex(void), init_keywords(void), expect(uint8_t tok);
extern uint8_t next(void);

#endif
