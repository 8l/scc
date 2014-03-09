
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "sizes.h"

#define NR_KEYW_HASH 16

unsigned char yytoken;
char yytext[IDENTSIZ + 1];
unsigned linenum;
unsigned columnum;
const char *filename;

struct keyword {
	char *str;
	unsigned char tok;
	unsigned char value;
	struct keyword *next;
};

static FILE *yyin;
static struct keyword *ktab[NR_KEYW_HASH];

struct symbol *yyval;

struct symbol *
integer(char *s, char base)
{
	static struct ctype *tp;
	register struct symbol *sym;
	static long long v;
	static char ch;

	tp = initctype(xmalloc(sizeof(*tp)));

type:	switch (ch = toupper(getc(yyin))) {
	case 'L':
		tp = ctype(tp, LONG);
		goto type;
	case 'U':
		tp = ctype(tp, UNSIGNED);
		goto type;
	default:
		ungetc(ch, yyin);
	}

	v = strtoll(s, NULL, base);
	sym = lookup(NULL, NS_IDEN);
	sym->ctype = *tp;

	switch (tp->type) {
	case INT:
		sym->i = v;
		break;
	case SHORT:
		sym->s = v;
		break;
	case LONG:
		sym->l = xmalloc(sizeof(long));
		*sym->l = v;
		break;
	case LLONG:
		sym->ll = xmalloc(sizeof(long long));
		*sym->ll = v;
		break;
	}

	return sym;
}

static char
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

	for (bp = yytext; bp < yytext + IDENTSIZ; *bp++ = ch) {
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

end:	if (bp == yytext + IDENTSIZ)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);
	yyval = integer(yytext, base);

	return CONSTANT;
}

void
init_keywords(void)
{
	static struct keyword buff[] = {
		{"auto", STORAGE, AUTO},
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
		{"extern", STORAGE, EXTERN},
		{"float", TYPE, FLOAT},
		{"for", FOR, FOR},
		{"goto", GOTO, GOTO},
		{"if", IF, IF},
		{"int", TYPE, INT},
		{"_Imaginary", TYPE, IMAGINARY},
		{"long", TYPE, LONG},
		{"register", STORAGE, REGISTER},
		{"restricted", RESTRICT, RESTRICT},
		{"return", RETURN, RETURN},
		{"short", TYPE, SHORT},
		{"signed", TYPE, SIGNED},
		{"sizeof", SIZEOF, SIZEOF},
		{"static", STORAGE, STATIC},
		{"struct", TYPE, STRUCT},
		{"switch", SWITCH, SWITCH},
		{"typedef", STORAGE, TYPEDEF},
		{"union", TYPE, UNION},
		{"unsigned", TYPE, UNSIGNED},
		{"void", TYPE, VOID},
		{"volatile", TQUALIFIER, VOLATILE},
		{"while", WHILE, WHILE},
		{NULL, 0, 0},
	};
	register struct keyword *bp;

	for (bp = buff;  bp->str; ++bp) {
		register unsigned char h = hash(bp->str) & NR_KEYW_HASH-1;
		bp->next = ktab[h];
		ktab[h] = bp;
	}
}

static unsigned char
keyword(register char *s)
{
	register struct keyword *bp;
	static struct symbol sym;

	for (bp = ktab[hash(s) & NR_KEYW_HASH-1]; bp; bp = bp->next) {
		if (*s == *bp->str && !strcmp(bp->str, s)) {
			sym.c = bp->value;
			yyval = &sym;
			return bp->tok;
		}
	}
	return 0;
}

static unsigned char
iden(void)
{
	register char ch, *bp;
	register struct symbol *sym;
	static unsigned char tok;

	for (bp = yytext; bp < yytext + IDENTSIZ; *bp++ = ch) {
		if (!isalnum(ch = getc(yyin)) && ch != '_')
			break;
	}
	if (bp == yytext + IDENTSIZ)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);

	return (tok = keyword(yytext)) ? tok : IDEN;
}

static unsigned char
skip(void)
{
	register int c;
	extern char parser_out_home;

	while (isspace(c = getc(yyin))) {
		switch (c) {
		case '\n': ++linenum, columnum = 1; break;
		case '\t': columnum += 8;	    break;
		default:   ++columnum;		    break;
		}
	}
	if (c == EOF) {
		if (parser_out_home)
			error("Find EOF while parsing");
		return 0;
	}
	ungetc(c, yyin);
	return 1;
}

static unsigned char
follow(unsigned char op, unsigned char eq, unsigned char rep)
{
	register char c = getc(yyin);

	yytext[1] = c;
	yytext[2] = '\0';
	if (c == '=')
		return eq;
	else if (c == op && rep)
		return rep;

	yytext[1] = '\0';
	ungetc(c, yyin);
	return op;
}

static unsigned char
rel_shift(unsigned char op)
{
	static char tokens[2][3] = {
		{GE, SHL, SHL_EQ},
		{LE, SHR, SHR_EQ}};
	register char c = getc(yyin);
	register char *tp = tokens[op == '>'];

	yytext[1] = c;
	yytext[2] = '\0';
	if (c == '=') {
		return tp[0];
	} else if (c == op) {
		if ((c = getc(yyin)) == '=')  {
			yytext[2] = c;
			yytext[3] = '\0';
			return tp[2];
		}
		op = tp[1];
	} else {
		yytext[1] = '\0';
	}
	ungetc(c, yyin);
	return op;
}

static unsigned char
minus(void)
{
	register int c = getc(yyin);

	yytext[1] = c;
	switch (c) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default:
		yytext[1] = '\0';
		ungetc(c, yyin);
		return '-';
	}
}

static unsigned char
operator(void)
{
	register unsigned char c = getc(yyin);

	yytext[0] = c;
	yytext[1] = '\0';
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

void
next(void)
{
	register unsigned char c;

	if (!skip()) {
		yytoken = EOFTOK;
	} else {
		ungetc(c = getc(yyin), yyin);
		if (isalpha(c) || c == '_')
			yytoken = iden();
		else if (isdigit(c))
			yytoken = number();
		else
			yytoken = operator();
	}
}

unsigned char
ahead(void)
{
	register char c;

	if (!skip())
		return EOFTOK;
	ungetc(c = getc(yyin), yyin);
	return c;
}

char
accept(register unsigned char tok)
{
	if (yytoken == tok) {
		next();
		return 1;
	}
	return 0;
}

void
expect(register unsigned char tok)
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
		return;
	}
	if ((yyin = fopen(file, "r")) == NULL)
		die("file '%s' not found", file);
	filename = file;
	columnum = linenum = 1;
}
