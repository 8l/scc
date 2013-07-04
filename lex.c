
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "sizes.h"

union yyval yyval;
unsigned char yytoken;
char yytext[IDENTSIZ + 1];
unsigned linenum;
unsigned columnum;
const char *filename;

static FILE *yyin;

static char
number(void)
{
	register char *bp;
	register char ch;

	for (bp = yytext; bp < yytext + IDENTSIZ; *bp++ = ch) {
		if (!isdigit(ch = getc(yyin)))
			break;
	}
	if (bp == yytext + IDENTSIZ)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);
	yyval.sym = lookup(yytext, NS_ANY);
	yyval.sym->val = atoi(yytext);

	return CONSTANT;
}

static unsigned char
iden(void)
{
	register char ch, *bp;
	register struct symbol *sym;

	for (bp = yytext; bp < yytext + IDENTSIZ; *bp++ = ch) {
		if (!isalnum(ch = getc(yyin)) && ch != '_')
			break;
	}
	if (bp == yytext + IDENTSIZ)
		error("identifier too long %s", yytext);
	*bp = '\0';
	ungetc(ch, yyin);
	yyval.sym = lookup(yytext, NS_ANY);

	return (yyval.sym->ns == NS_KEYWORD) ? yyval.sym->tok : IDEN;
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

	skip();
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
