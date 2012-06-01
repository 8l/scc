
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "symbol.h"
#include "tokens.h"


#define NR_KWD_HASH 32

static struct keyword {
	char *str;
	unsigned char tok;
	struct keyword *next;
} keywords [] = {"auto", AUTO, NULL,
		 "break", BREAK, NULL,
		 "_Bool", CHAR, NULL,
		 "case", CASE, NULL,
		 "char", CHAR, NULL,
		 "const", CONST, NULL,
		 "continue", CONTINUE, NULL,
		 "default", DEFAULT, NULL,
		 "do", DO, NULL,
		 "double", DOUBLE, NULL,
		 "else", ELSE, NULL,
		 "enum", ENUM, NULL,
		 "extern", EXTERN, NULL,
		 "float", FLOAT, NULL,
		 "for", FOR, NULL,
		 "goto", GOTO, NULL,
		 "if", IF, NULL,
		 "int", INT, NULL,
		 "long", LONG, NULL,
		 "register", REGISTER, NULL,
		 "restricted", RESTRICTED, NULL,
		 "return", RETURN, NULL,
		 "short", SHORT, NULL,
		 "signed", SIGNED, NULL,
		 "sizeof", SIZEOF, NULL,
		 "static", STATIC, NULL,
		 "struct", STRUCT, NULL,
		 "switch", SWITCH, NULL,
		 "typedef", TYPEDEF, NULL,
		 "union", UNION, NULL,
		 "unsigned", UNSIGNED, NULL,
		 "void", VOID, NULL,
		 "volatile", VOLATILE, NULL,
		 "while", WHILE, NULL,
		 NULL, 0, NULL
};

static struct keyword *khash[NR_KWD_HASH];
static FILE *yyin;

unsigned char yytoken;
unsigned char yyhash;
char yytext[TOKSIZ_MAX + 1];
unsigned linenum;
unsigned columnum;
const char *filename;


static unsigned char hashfun(register const char *s)
{
	register unsigned char h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h;
}



void init_lex(void)
{
	register struct keyword *bp;
	static unsigned char h;

	for (bp = keywords; bp->str; bp++) {
		register struct keyword *aux, *ant;
		h = hashfun(bp->str) & (NR_KWD_HASH - 1);
		if (!(aux = khash[h]) || strcmp(bp->str, aux->str) < 0) {
			khash[h] = bp;
			bp->next = aux;
			continue;
		}
		for (ant = aux; aux; ant = aux, aux = aux->next) {
			if (strcmp(bp->str, aux->str) < 0)
				break;
		}
		ant->next = bp;
		bp->next = aux;
	}
}

static unsigned char iden(void)
{
	register struct keyword *kwp;
	register char ch;
	register char *bp = yytext;

	for (yyhash = 0; bp < yytext + TOKSIZ_MAX; *bp++ = ch) {
		if (!isalnum(ch = getc(yyin)) && ch != '_')
			break;
		yyhash += ch;
	}
	if (bp == yytext + TOKSIZ_MAX)
		error("identifier too long %s", yytext);
	ungetc(ch, yyin);
	*bp = '\0';
	yyhash &= NR_KWD_HASH - 1;
	for (kwp = khash[yyhash]; kwp; kwp = kwp->next) {
		if (!strcmp(kwp->str, yytext))
			return kwp->tok;
	}
	return IDENTIFIER;
}



unsigned char next(void)
{
	static unsigned int c;
	register unsigned char ch;
	extern char parser_out_home;

	while (isspace(c = getc(yyin)))
		/* nothing */;
	if (c == EOF) {
		if (parser_out_home)
			error("Find EOF while parsing");
		else
			return EOFTOK;
	}
	ch = c;
	if (isalpha(ch) || ch == '_') {
		ungetc(ch, yyin);
		ch = iden();
	} else if (isdigit(ch)) {
		;
	} else {
		switch (ch) {
		case '&': case '|':
			if ((c = getc(yyin)) == ch) {
				yytext[1] = yytext[0] = ch;
				yytext[2] = '\0';
				ch |= 0x80; /* TODO */
				break;
			} else {
				ungetc(c, yyin);
			}
		case '^': case '=': case '<': case '>':
		case '*': case '+': case '-': case '/':
			if ((c = getc(yyin)) == '=') {
				yytext[0] = ch;
				yytext[1] = c;
				yytext[2] = '\0';
				ch |= 0x80; /* TODO */
				break;
			} else {
				ungetc(c, yyin);
			}
		case ';': case '{': case '}': case '(': case ')': case '~':
		case '!': case ',': case '?': case '[': case ']': case ':':
			yytext[0] = ch;
			yytext[1] = '\0';
			break;
		default:
			error("Incorrect character '%02x", c);
		}
	}

return_token:
	printf("Token = %c (%u, '%s')\n",
	       (isprint(ch)) ? ch : ' ', (unsigned) ch, yytext);
	return yytoken = ch;
}

char accept(unsigned char tok)
{
	if (yytoken == tok) {
		next();
		return 1;
	}
	return 0;
}

void expect(unsigned char tok)
{
	if (yytoken != tok)
		error("unexpected %s", yytext);
	next();
}

void open_file(const char *file)
{
	if (yyin != NULL)
		fclose(yyin);
	if (file == NULL) {
		yyin = stdin;
		filename = "(stdin)";
		return;
	}
	if ((yyin = fopen(file, "r")) == NULL)
		die("file '%s' not found", file);
	filename = file;
}
