#include <stddef.h>

#include "tokens.h"
#include "symbol.h"


static struct keyword {
	char *str;
	unsigned char tok;
} keywords [] = {"auto", AUTO,
		 "break", BREAK,
		 "_Bool", CHAR,
		 "case", CASE,
		 "char", CHAR,
		 "const", CONST,
		 "continue", CONTINUE,
		 "default", DEFAULT,
		 "do", DO,
		 "double", DOUBLE,
		 "else", ELSE,
		 "enum", ENUM,
		 "extern", EXTERN,
		 "float", FLOAT,
		 "for", FOR,
		 "goto", GOTO,
		 "if", IF,
		 "int", INT,
		 "long", LONG,
		 "register", REGISTER,
		 "restricted", RESTRICT,
		 "return", RETURN,
		 "short", SHORT,
		 "signed", SIGNED,
		 "sizeof", SIZEOF,
		 "static", STATIC,
		 "struct", STRUCT,
		 "switch", SWITCH,
		 "typedef", TYPEDEF,
		 "union", UNION,
		 "unsigned", UNSIGNED,
		 "void", VOID,
		 "volatile", VOLATILE,
		 "while", WHILE,
		 NULL, 0,
};

void
init_keywords(void)
{
	register struct keyword *bp;
	register struct symbol *sym;

	for (bp = keywords; bp->str; bp++) {
		sym = lookup(bp->str, NS_KEYWORD, CTX_ANY);
		sym->tok = bp->tok;
	}
	new_ctx();
}
