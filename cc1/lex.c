/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"

unsigned yytoken;
struct yystype yylval;
char yytext[STRINGSIZ+3];
unsigned short yylen;
int cppoff;
int lexmode = CCMODE;

int namespace = NS_IDEN;
static int safe, eof;
Input *input;

static void
allocinput(char *fname, FILE *fp)
{
	Input *ip;

	ip = xmalloc(sizeof(Input));
	ip->fname = xstrdup(fname);
	ip->p = ip->begin = ip->line = xmalloc(INPUTSIZ);
	ip->p[0] = '\0';
	ip->nline = 0;
	ip->next = input;
	ip->fp = fp;
	input = ip;
}

void
ilex(char *fname)
{
	static struct keyword keys[] = {
		{"auto", SCLASS, AUTO},
		{"break", BREAK, BREAK},
		{"_Bool", TYPE, BOOL},
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
		{"inline", TQUALIFIER, INLINE},
		{"int", TYPE, INT},
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
	FILE *fp;

	if (!fname) {
		fp = stdin;
		fname = "<stdin>";
	} else {
		if ((fp = fopen(fname, "r")) == NULL) {
			die("error: failed to open input file '%s': %s",
			    fname, strerror(errno));
		}
	}
	allocinput(fname, fp);
	keywords(keys, NS_KEYWORD);
}

int
addinput(char *fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;
	allocinput(fname, fp);
	return 1;
}

static void
delinput(void)
{
	Input *ip = input;

	if (!ip->next)
		eof = 1;
	if (fclose(ip->fp))
		die("error: failed to read from input file '%s'", ip->fname);
	if (eof)
		return;
	input = ip->next;
	free(ip->fname);
	free(ip->line);
}

static void
newline(void)
{
	if (++input->nline == 0)
		die("error: input file '%s' too long", input->fname);
}

static char
readchar(void)
{
	int c;
	FILE *fp;

repeat:
	fp = input->fp;

	switch (c = getc(fp)) {
	case EOF:
		c = '\0';
		break;
	case '\\':
		if ((c = getc(fp)) == '\n') {
			newline();
			goto repeat;
		}
		ungetc(c, fp);
		c = '\\';
		break;
	case '\n':
		newline();
		break;
	}

	return c;
}

static void
comment(char type)
{
	if (type == '*') {
		while (!eof) {
			while (readchar() != '*' && !eof)
				/* nothing */;
			if (readchar() == '/')
				break;
		}
	} else {
		while (readchar() != '\n' && !eof)
			/* nothing */;
	}
	if (eof)
		error("unterminated comment");
}

static int
readline(void)
{
	char *bp, *lim;
	char c, peekc = 0;

repeat:
	input->begin = input->p = input->line;
	*input->line = '\0';
	if (eof)
		return 0;
	if (feof(input->fp)) {
		delinput();
		goto repeat;
	}
	lim = &input->line[INPUTSIZ-1];
	for (bp = input->line; bp < lim; *bp++ = c) {
		c = (peekc) ? peekc : readchar();
		peekc = 0;
		if (c == '\n' || c == '\0')
			break;
		if (c != '/')
			continue;
		if ((c = readchar()) != '*' && c != '/') {
			peekc = c;
			c = '/';
		} else {
			comment(c);
			c = ' ';
		}
	}

	if (bp == lim)
		error("line too long");
	*bp = '\0';
	return 1;
}

int
moreinput(void)
{
	static char file[FILENAME_MAX];
	static unsigned nline;
	char *s;

repeat:
	if (!readline())
		return 0;
	while (isspace(*input->p))
		++input->p;
	input->begin = input->p;
	if (*input->p == '\0' || cpp() || cppoff) {
		*input->begin = '\0';
		goto repeat;
	}

	if (onlycpp) {
		putchar('\n');
		if (strcmp(file, input->fname)) {
			strcpy(file, input->fname);
			s = "#line %u %s\n";
		} else if (nline+1 != input->nline) {
			s = "#line %u\n";
		} else {
			s = "";
		}
		nline = input->nline;
		printf(s, nline, file);
	}
	input->begin = input->p;
	return 1;
}

static void
tok2str(void)
{
	if ((yylen = input->p - input->begin) > INTIDENTSIZ)
		error("token too big");
	strncpy(yytext, input->begin, yylen);
	yytext[yylen] = '\0';
	input->begin = input->p;
}

static Symbol *
readint(char *s, int base, int sign, Symbol *sym)
{
	Type *tp = sym->type;
	struct limits *lim;
	TUINT u, val, max;
	int c;

	lim = getlimits(tp);
	max = lim->max.i;
	if (*s == '0')
		++s;
	if (toupper(*s) == 'X')
		++s;

	for (u = 0; isxdigit(c = *s++); u = u*base + val) {
		static char letters[] = "0123456789ABCDEF";
		val = strchr(letters, toupper(c)) - letters;
	repeat:
		if (u <= max/base && u*base <= max - val)
			continue;
		if (tp->prop & TSIGNED) {
			if (tp == inttype)
				tp = (base==10) ? longtype : uinttype;
			else if (tp == longtype)
				tp = (base==10) ? llongtype : ulongtype;
			else
				goto overflow;
		} else {
			if (tp == uinttype)
				tp = (sign==UNSIGNED) ? ulongtype : longtype;
			else if (tp == ulongtype)
				tp = (sign==UNSIGNED) ? ullongtype : llongtype;
			else
				goto overflow;
		}
		sym->type = tp;
		lim = getlimits(tp);
		max = lim->max.i;
		goto repeat;
	}

	if (tp->prop & TSIGNED)
		sym->u.i = u;
	else
		sym->u.u = u;

	return sym;

overflow:
	errorp("overflow in integer constant");
	return sym;
}

static unsigned
integer(char *s, char base)
{
	Type *tp;
	Symbol *sym;
	unsigned size, sign;

	for (size = sign = 0; ; ++input->p) {
		switch (toupper(*input->p)) {
		case 'L':
			if (size == LLONG)
				goto wrong_type;
			size = (size == LONG) ? LLONG : LONG;
			continue;
		case 'U':
			if (sign == UNSIGNED)
				goto wrong_type;
			sign = UNSIGNED;
			continue;
		default:
			goto convert;
		wrong_type:
			error("invalid suffix in integer constant");
		}
	}

convert:
	tp = ctype(INT, sign, size);
	sym = newsym(NS_IDEN);
	sym->type = tp;
	sym->flags |= SCONSTANT;
	yylval.sym = readint(s, base, sign, sym);
	return CONSTANT;
}

static char *
digits(unsigned base)
{
	char c, *p;

	for (p = input->p; c = *p; ++p) {
		switch (base) {
		case 8:
			if (!strchr("01234567", c))
				goto end;
			break;
		case 10:
			if (!isdigit(c))
				goto end;
			break;
		case 16:
			if (!isxdigit(c))
				goto end;
			break;
		}
	}
end:
	input->p = p;
	tok2str();
	return yytext;
}

static unsigned
number(void)
{
	char base;

	if (*input->p != '0') {
		base = 10;
	} else {
		if (toupper(*++input->p) == 'X') {
			++input->p;
			base = 16;
		} else {
			base = 8;
		}
	}

	return integer(digits(base), base);
}

static char
escape(void)
{
	int c, base;

	switch (*++input->p) {
	case 'a':  return '\a';
	case 'f':  return '\f';
	case 'n':  return '\n';
	case 'r':  return '\r';
	case 't':  return '\t';
	case 'v':  return '\v';
	case '"':  return '"';
	case '\'': return '\'';
	case '\\': return '\\';
	case '\?': return '\?';
	case 'u':
		/*
		 * FIXME: universal constants are not correctly handled
		 */
		if (!isdigit(*++input->p))
			warn("incorrect digit for numerical character constant");
		base = 10;
		break;
	case 'x':
		if (!isxdigit(*++input->p))
			warn("\\x used with no following hex digits");
		base = 16;
		break;
	case '0':
		if (!strchr("01234567", *++input->p))
			warn("\\0 used with no following octal digits");
		base = 8;
		break;
	default:
		warn("unknown escape sequence");
		return ' ';
	}
	errno = 0;
	c = strtoul(input->p, &input->p, base);
	if (errno || c > 255)
		warn("character constant out of range");
	--input->p;
	return c;
}

static unsigned
character(void)
{
	static char c;
	Symbol *sym;

	if ((c = *++input->p) == '\\')
		c = escape();
	else
		c = *input->p;
	++input->p;
	if (*input->p != '\'')
		error("invalid character constant");
	else
		++input->p;

	sym = newsym(NS_IDEN);
	sym->u.i = c;
	sym->type = inttype;
	yylval.sym = sym;
	return CONSTANT;
}

static unsigned
string(void)
{
	char *bp = yytext, c;

	*bp++ = '"';
repeat:
	for (++input->p; (c = *input->p) != '"'; ++input->p) {
		if (c == '\0')
			error("missing terminating '\"' character");
		if (c == '\\')
			c = escape();
		if (bp == &yytext[STRINGSIZ+1])
			error("string too long");
		*bp++ = c;
	}

	input->begin = ++input->p;
	if (ahead() == '"')
		goto repeat;
	*bp = '\0';

	yylen = bp - yytext + 1;
	yylval.sym = newstring(yytext+1, yylen-1);
	*bp++ = '"';
	*bp = '\0';
	return STRING;
}

static unsigned
iden(void)
{
	Symbol *sym;
	char *p, *begin;

	begin = input->p;
	for (p = begin; isalnum(*p) || *p == '_'; ++p)
		/* nothing */;
	input->p = p;
	tok2str();
	sym = lookup(namespace, yytext);
	if (sym->ns == NS_CPP) {
		if (!disexpand && expand(begin, sym))
			return next();
		/*
		 * it is not a correct macro call, so try to find
		 * another definition.
		 */
		if (lexmode != CPPMODE)
			sym = nextsym(sym, namespace);
	}
	yylval.sym = sym;
	if (sym->flags & SCONSTANT)
		return CONSTANT;
	if (sym->token != IDEN)
		yylval.token = sym->u.token;
	return sym->token;
}

static unsigned
follow(int expect, int ifyes, int ifno)
{
	if (*input->p++ == expect)
		return ifyes;
	--input->p;
	return ifno;
}

static unsigned
minus(void)
{
	switch (*input->p++) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default: --input->p; return '-';
	}
}

static unsigned
plus(void)
{
	switch (*input->p++) {
	case '+': return INC;
	case '=': return ADD_EQ;
	default: --input->p; return '+';
	}
}

static unsigned
relational(int op, int equal, int shift, int assig)
{
	char c;

	if ((c = *input->p++) == '=')
		return equal;
	if (c == op)
		return follow('=', assig, shift);
	--input->p;
	return op;
}

static unsigned
logic(int op, int equal, int logic)
{
	char c;

	if ((c = *input->p++) == '=')
		return equal;
	if (c == op)
		return logic;
	--input->p;
	return op;
}

static unsigned
dot(void)
{
	char c;

	if ((c = *input->p) != '.')
		return '.';
	if ((c = *++input->p) != '.')
		error("incorrect token '..'");
	++input->p;
	return ELLIPSIS;
}

static unsigned
operator(void)
{
	unsigned t;

	switch (t = *input->p++) {
	case '<': t = relational('<', LE, SHL, SHL_EQ); break;
	case '>': t = relational('>', GE, SHR, SHR_EQ); break;
	case '&': t = logic('&', AND_EQ, AND); break;
	case '|': t = logic('|', OR_EQ, OR); break;
	case '=': t = follow('=', EQ, '='); break;
	case '^': t = follow('=', XOR_EQ, '^'); break;
	case '*': t = follow('=', MUL_EQ, '*'); break;
	case '/': t = follow('=', DIV_EQ, '/'); break;
	case '!': t = follow('=', NE, '!'); break;
	case '#': t = follow('#', '$', '#'); break;
	case '-': t = minus(); break;
	case '+': t = plus(); break;
	case '.': t = dot(); break;
	}
	tok2str();
	return t;
}

/* TODO: Ensure that namespace is NS_IDEN after a recovery */

static void
skipspaces(void)
{
repeat:
	while (isspace(*input->p))
		++input->p;
	if (*input->p == '\0' && lexmode != CPPMODE) {
		if (!moreinput())
			return;
		goto repeat;
	}
	input->begin = input->p;
}

unsigned
next(void)
{
	char c;

	skipspaces();
	c = *input->begin;
	if ((eof || lexmode == CPPMODE) && c == '\0') {
		strcpy(yytext, "<EOF>");
		if (cppctx && eof)
			error("#endif expected");
		yytoken = EOFTOK;
		goto exit;
	}

	if (isalpha(c) || c == '_')
		yytoken = iden();
	else if (isdigit(c))
		yytoken = number();
	else if (c == '"')
		yytoken = string();
	else if (c == '\'')
		yytoken = character();
	else
		yytoken = operator();

exit:
	DBG("TOKEN %s", yytext);
	return yytoken;
}

void
expect(unsigned tok)
{
	if (yytoken != tok) {
		if (isgraph(tok))
			errorp("expected '%c' before '%s'", tok, yytext);
		else
			errorp("unexpected '%s'", yytext);
	} else {
		next();
	}
}

char
ahead(void)
{
	skipspaces();
	return *input->begin;
}

void
setsafe(int type)
{
	safe = type;
}

void
discard(void)
{
	extern jmp_buf recover;
	char c;

	input->begin = input->p;
	for (c = yytoken; ; c = *input->begin++) {
		switch (safe) {
		case END_COMP:
			if (c == '}')
				goto jump;
			goto semicolon;
		case END_COND:
			if (c == ')')
				goto jump;
			break;
		case END_LDECL:
			if (c == ',')
				goto jump;
		case END_DECL:
		semicolon:
			if (c == ';')
				goto jump;
			break;
		}
		if (c == '\0' && !moreinput())
			exit(1);
	}
jump:
	yytoken = c;
	longjmp(recover, 1);
}
