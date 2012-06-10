
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "types.h"


static FILE *yyin;
union yyval yyval;
unsigned char yytoken;
unsigned char yyhash;
char yytext[TOKSIZ_MAX + 1];
unsigned linenum;
unsigned columnum;
const char *filename;


static char number(void)
{
	register char *bp;
	register char ch;

	for (bp = yytext; bp < yytext + TOKSIZ_MAX; *bp++ = ch) {
		if (!isdigit(ch = getc(yyin)))
			break;
	}
	if (bp == yytext + TOKSIZ_MAX)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);

	return CONSTANT;
}

static unsigned char iden(void)
{
	register char ch;
	register char *bp = yytext;
	register struct symbol *sym;

	for (yyhash = 0; bp < yytext + TOKSIZ_MAX; *bp++ = ch) {
		if (!isalnum(ch = getc(yyin)) && ch != '_')
			break;
		yyhash += ch;
	}
	if (bp == yytext + TOKSIZ_MAX)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);
	if ((sym = lookup(yytext, yyhash)) && sym->type == T_KWD)
		return sym->tok;
	yyval.sym = sym;
	return IDEN;
}

static unsigned char skip(void)
{
	register int c;
	extern char parser_out_home;

	while (isspace(c = getc(yyin))) {
		if (c == '\n')
			++linenum, columnum = 1;
		else
			++columnum;
	}
	if (c == EOF) {
		if (parser_out_home)
			error("Find EOF while parsing");
		return 1;
	}
	ungetc(c, yyin);
	return 0;
}

static unsigned char
follow(unsigned char op, unsigned char eq, unsigned char rep)
{
	register char c;

	if ((c = getc(yyin)) == '=')
		return eq;
	else if (c == op && rep)
		return rep;
	ungetc(c, yyin);
	return op;
}

static unsigned char rel_shift(unsigned char op)
{
	static char tokens[2][3] = {
		{GE, LSHIFT, LSHIFT_EQ},
		{LE, RSHIFT, RSHIFT_EQ}};
	register char c;
	register char *tp = tokens[op == '>'];

	if ((c = getc(yyin)) == '=') {
		return tp[0];
	} else if (c == op) {
		if ((c = getc(yyin)) == '=')
			return tp[2];
		op = tp[1];
	}
	ungetc(c, yyin);
	return c;
}

static unsigned char minus(void)
{
	register int c;

	switch (c = getc(yyin)) {
	case '-': return DEC;
	case '>': return PTR;
	case '=': return SUB_EQ;
	default:
		ungetc(c, yyin);
		return '-';
	}
}

unsigned char next(void)
{
	register unsigned char c;

	if (!skip())
		c = EOFTOK;
	if (isalpha(c = getc(yyin)) || c == '_') {
		ungetc(c, yyin);
		c = iden();
	} else if (isdigit(c)) {
		ungetc(c, yyin);
		c = number();
	} else {
		switch (c) {
		case '=': c = follow('=', EQ, 0); break;
		case '^': c = follow('^', XOR_EQ, 0); break;
		case '*': c = follow('*', MUL_EQ, 0); break;
		case '!': c = follow('!', NE, 0); break;
		case '+': c = follow('+', ADD_EQ, INC); break;
		case '&': c = follow('&', AND_EQ, AND); break;
		case '|': c = follow('|', OR_EQ, OR); break;
		case '<': c = rel_shift('<'); break;
		case '>': c = rel_shift('>'); break;
		case '-': c = minus(); break;
		}
	}
	return yytoken = c;
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
	columnum = linenum = 1;
}
