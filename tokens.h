#ifndef TOKENS_H
#define TOKENS_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif


/* Don't change this codification because program used it!!! */
enum tokens {
	/* types */
	INT = 1, CHAR, FLOAT, LONG, LLONG, SHORT, VOID, DOUBLE,
	LDOUBLE, STRUCT, UNION, ENUM, BOOL, ARY, PTR, FTN,
	COMPLEX, IMAGINARY, BITFLD,
	/* storage specifier */
	TYPEDEF, EXTERN, STATIC, AUTO, REGISTER,
	/* type qualifier */
	VOLATILE, CONST, RESTRICT,
	/* sign specifier */
	UNSIGNED, SIGNED,
	/* other tokens */
	IDEN = 128, CONSTANT, SIZEOF,
	INDIR, INC, DEC, SHL, SHR,
	LE, GE, EQ, NE, AND, OR,
	MUL_EQ, DIV_EQ, MOD_EQ, ADD_EQ, SUB_EQ, AND_EQ,
	XOR_EQ, OR_EQ, SHL_EQ, SHR_EQ,
	TYPE_NAME, ELLIPSIS,
	CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO,
	CONTINUE, BREAK, RETURN, EOFTOK, NOTOK
};


struct symbol;
extern struct symbol *yyval;

extern char yytext[];
extern size_t yylen;
extern unsigned char yytoken;


extern void init_lex(void);
extern void next(void);
extern char accept(unsigned char tok);
extern void expect(unsigned char tok);
extern void init_keywords(void);
extern unsigned char ahead(void);
#endif
