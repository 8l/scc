#ifndef TOKENS_H
#define TOKENS_H



#define TOKSIZ_MAX 21

/* Don't change this codification because program used it!!! */
enum {
  /* types */
  INT = 1, CHAR, FLOAT, LONG, LLONG, SHORT, VOID, DOUBLE,
  LDOUBLE, STRUCT, UNION, ENUM, UTYPE, BOOL,
  /* storage specifier */
  TYPEDEF, EXTERN, STATIC, AUTO, REGISTER,
  /* type qualifier */
  VOLATILE, CONST, RESTRICTED,
  /* sign specifier */
  UNSIGNED, SIGNED
};




enum {
	IDENTIFIER = 128, CONSTANT, STRING_LITERAL, SIZEOF,
	PTR_OP, INC_OP, DEC_OP, LSHIFT_OP, RSHIFT_OP,
	LE_OP, GE_OP, EQ_OP, NE_OP,
	AND_OP, OR_OP, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, ADD_ASSIGN,
	SUB_ASSIGN, LSHIFT_ASSIGN, RSHIFT_ASSIGN, AND_ASSIGN,
	XOR_ASSIGN, OR_ASSIGN, TYPE_NAME,
	ELLIPSIS,
	CASE, DEFAULT, IF, ELSE, SWITCH, WHILE, DO, FOR, GOTO,
	CONTINUE, BREAK, RETURN, EOFTOK
};



union yyval {
	struct symbol *sym;
} yyval;



extern char yytext[];
extern unsigned char yyhash;
extern size_t yylen;
extern unsigned char yytoken;
extern union yyval yyval;


extern void init_lex(void);
extern unsigned char next(void);
extern char accept(unsigned char tok);
extern void expect(unsigned char tok);
#endif
