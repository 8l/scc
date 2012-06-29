#ifndef TOKENS_H
#define TOKENS_H



#define TOKSIZ_MAX 21

/* Don't change this codification because program used it!!! */
enum tokens {
	/* types */
	INT = 1, CHAR, FLOAT, LONG, LLONG, SHORT, VOID, DOUBLE,
	LDOUBLE, STRUCT, UNION, ENUM, UTYPE, BOOL,
	/* storage specifier */
	TYPEDEF, EXTERN, STATIC, AUTO, REGISTER,
	/* type qualifier */
	VOLATILE, CONST, RESTRICT,
	/* sign specifier */
	UNSIGNED, SIGNED,
	/* other tokens */
	IDEN = 128, CONSTANT, SIZEOF,
	PTR, INC, DEC, SHL, SHR,
	LE, GE, EQ, NE, AND, OR,
	MUL_EQ, DIV_EQ, MOD_EQ, ADD_EQ, SUB_EQ, AND_EQ,
	XOR_EQ, OR_EQ, SHL_EQ, SHR_EQ,
	TYPE_NAME, ELLIPSIS,
	CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO,
	CONTINUE, BREAK, RETURN, EOFTOK, NOTOK
};


struct symbol;
union yyval {
	struct symbol *sym;
};


extern union yyval yyval;
extern char yytext[];
extern unsigned char yyhash;
extern size_t yylen;
extern unsigned char yytoken;
extern union yyval yyval;


extern void init_lex(void);
extern void next(void);
extern char accept(unsigned char tok);
extern void expect(unsigned char tok);
extern void init_keywords(void);
extern unsigned char ahead(void);
#endif
