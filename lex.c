
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "sizes.h"

static FILE *yyin;
const char *filename;
unsigned linenum;
unsigned columnum;

uint8_t yytoken, yyntoken;;
union yystype yylval;
char yytext[IDENTSIZ + 1];

static union yystype yynlval;
static char yybuf[IDENTSIZ + 1];

static uint8_t
integer(char *s, char base)
{
	static struct ctype *tp;
	static char ch;

	/* TODO: implement again */

type:	switch (ch = toupper(getc(yyin))) {
	case 'L':
		goto type;
	case 'U':
		goto type;
	default:
		ungetc(ch, yyin);
	}

	return CONSTANT;
}

static uint8_t
number(void)
{
	register char *bp, ch;
	static char base, type, sign;

	if ((ch = getc(yyin)) == '0') {
		if (toupper(ch = getc(yyin)) == 'X') {
			base = 16;
		} else {
			base = 8;
			ungetc(ch, yyin);
		}
	} else {
		base = 10;
		ungetc(ch, yyin);
	}

	for (bp = yybuf; bp < &yybuf[IDENTSIZ]; *bp++ = ch) {
		ch = getc(yyin);
		switch (base) {
		case 8:
			if (ch >= '7')
				goto end;
			/* passthru */
		case 10:
			if (!isdigit(ch))
				goto end;
			break;
		case 16:
			if (!isxdigit(ch))
				goto end;
			break;
		}
	}

end:	if (bp == &yybuf[IDENTSIZ])
		error("identifier too long %s", yybuf);
	*bp = '\0';
	ungetc(ch, yyin);
	return integer(yybuf, base);
}

void
init_keywords(void)
{
	static struct {
		char *str;
		uint8_t token, value;
	} *bp, buff[] = {
		{"auto", SCLASS, AUTO},
		{"break", BREAK, BREAK},
		{"_Bool", TYPE, BOOL},
		{"_Complex", TYPE, COMPLEX},
		{"case", CASE, CASE},
		{"char", TYPE, CHAR},
		{"const", TQUALIFIER, CONST},
		{"continue", CONTINUE, CONTINUE},
		{"default", DEFAULT, DEFAULT},
		{"do", DO, DO},
		{"double", TYPE, DOUBLE},
		{"else", ELSE, ELSE},
		{"enum", TYPE, ENUM},
		{"extern", SCLASS, EXTERN},
		{"float", TYPE, FLOAT},
		{"for", FOR, FOR},
		{"goto", GOTO, GOTO},
		{"if", IF, IF},
		{"int", TYPE, INT},
		{"_Imaginary", TYPE, IMAGINARY},
		{"long", TYPE, LONG},
		{"register", SCLASS, REGISTER},
		{"restrict", TQUALIFIER, RESTRICT},
		{"return", RETURN, RETURN},
		{"short", TYPE, SHORT},
		{"signed", TYPE, SIGNED},
		{"sizeof", SIZEOF, SIZEOF},
		{"static", SCLASS, STATIC},
		{"struct", TYPE, STRUCT},
		{"switch", SWITCH, SWITCH},
		{"typedef", SCLASS, TYPEDEF},
		{"union", TYPE, UNION},
		{"unsigned", TYPE, UNSIGNED},
		{"void", TYPE, VOID},
		{"volatile", TQUALIFIER, VOLATILE},
		{"while", WHILE, WHILE},
		{NULL, 0, 0},
	};
	register struct symbol *sym;

	for (bp = buff; bp->str; ++bp) {
		sym = install(bp->str, NS_KEYWORD);
		sym->token = bp->token;
		sym->u.token = bp->value;
	}
}

static uint8_t
iden(void)
{
	register char *bp;
	register int c;
	struct symbol *sym;

	for (bp = yybuf; bp < &yybuf[IDENTSIZ]; *bp++ = c) {
		if (!isalnum(c = getc(yyin)) && c != '_')
			break;
	}
	if (bp == &yybuf[IDENTSIZ])
		error("identifier too long %s", yybuf);
	*bp = '\0';
	ungetc(c, yyin);

	sym = lookup(yybuf, NS_IDEN);
	if (!sym || sym->token == IDEN) {
		yynlval.sym = sym;
		return IDEN;
	}
	yynlval.token = sym->u.token;
	return sym->token;
}

static uint8_t
follow(uint8_t op, uint8_t eq, uint8_t rep)
{
	register int c = getc(yyin);

	yybuf[1] = c;
	yybuf[2] = '\0';
	if (c == '=')
		return eq;
	else if (c == op && rep)
		return rep;

	yybuf[1] = '\0';
	ungetc(c, yyin);
	return op;
}

static uint8_t
rel_shift(uint8_t op)
{
	static uint8_t tokens[2][3] = {
		{GE, SHL, SHL_EQ},
		{LE, SHR, SHR_EQ}
	};
	register int c = getc(yyin);
	register char *tp = tokens[op == '>'];

	yybuf[1] = c;
	yybuf[2] = '\0';
	if (c == '=') {
		return tp[0];
	} else if (c == op) {
		if ((c = getc(yyin)) == '=')  {
			yybuf[2] = c;
			yybuf[3] = '\0';
			return tp[2];
		}
		op = tp[1];
	} else {
		yybuf[1] = '\0';
	}
	ungetc(c, yyin);
	return op;
}

static uint8_t
minus(void)
{
	register int c = getc(yyin);

	yybuf[1] = c;
	switch (c) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default:
		yybuf[1] = '\0';
		ungetc(c, yyin);
		return '-';
	}
}

static uint8_t
operator(void)
{
	register uint8_t c = getc(yyin);

	yybuf[0] = c;
	yybuf[1] = '\0';
	switch (c) {
	case '=': return follow('=', EQ, 0);
	case '^': return follow('^', XOR_EQ, 0);
	case '*': return follow('*', MUL_EQ, 0);
	case '!': return follow('!', NE, 0);
	case '+': return follow('+', ADD_EQ, INC);
	case '&': return follow('&', AND_EQ, AND);
	case '|': return follow('|', OR_EQ, OR);
	case '<': return rel_shift('<');
	case '>': return rel_shift('>');
	case '-': return minus();
	default: return c;
	}
}

uint8_t
next(void)
{
	static int c;
	extern int8_t forbid_eof;

	strcpy(yytext, yybuf);
	yylval = yynlval;
	if ((yytoken = yyntoken) == EOFTOK) {
		if (forbid_eof)
			error("Find EOF while parsing");
		goto ret;
	}

	while (isspace(c = getc(yyin))) {
		switch (c) {
		case '\n': ++linenum, columnum = 1; break;
		case '\t': columnum += 8;	    break;
		default:   ++columnum;		    break;
		}
	}

	if (c == EOF) {
		yyntoken = EOFTOK;
		goto ret;
	}

	ungetc(c, yyin);
	if (isalpha(c) || c == '_')
		yyntoken = iden();
	else if (isdigit(c))
		yyntoken = number();
	else
		yyntoken = operator();

ret:	return yytoken;
}

void
expect(register uint8_t tok)
{
	if (yytoken != tok)
		error("unexpected %s", yytext);
	next();
}

void
open_file(register const char *file)
{
	if (yyin != NULL)
		fclose(yyin);
	if (file == NULL) {
		yyin = stdin;
		filename = "(stdin)";
	} else {
		if ((yyin = fopen(file, "r")) == NULL)
			die("file '%s' not found", file);
		filename = file;
	}
	columnum = linenum = 1;
	next();      /* prefetch first token */
}
