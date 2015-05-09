
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define INPUTSIZ 120

typedef struct input Input;

struct input {
	char *fname;
	unsigned short nline;
	FILE *fp;
	char *line, *begin, *p;
	struct input *next;
};

uint8_t lex_ns = NS_IDEN;

uint8_t yytoken;
struct yystype yylval;
char yytext[IDENTSIZ + 1];
static uint8_t safe;
static Input *input;

bool
addinput(char *fname)
{
	Input *ip;
	FILE *fp;
	unsigned short nline = 1;

	if (fname) {
		if ((fp = fopen(fname, "r")) == NULL)
			return 0;
		fname = xstrdup(fname);
	} else if (!input) {
		fp = stdin;
		fname = "(stdin)";
	} else {
		fname = input->fname;
		nline = input->nline;
		fp = NULL;
	}

	ip = xmalloc(sizeof(Input));
	ip->fname = fname;
	ip->next = input;
	ip->line = NULL;
	ip->nline = nline;
	ip->fp = fp;
	input = ip;
	return 1;
}

void
delinput(void)
{
	Input *ip = input;
	FILE *fp = ip->fp;

	if (fp) {
		if (fclose(fp))
			die("error reading from input file '%s'", ip->fname);
		if (ip->fp != stdin)
			free(ip->fname);
	}
	input = ip->next;
	free(ip->line);
	free(ip);
}

char *
filename(void)
{
	return input->fname;
}

unsigned short
fileline(void)
{
	return input->nline;
}

/* TODO: preprocessor error must not rise recover */
char *
include(char *s)
{
	char fname[FILENAME_MAX], delim, c, *p;
	size_t len;

	if ((c = *s++) == '>')
		delim = '>';
	else if (c == '"')
		delim = '"';
	else
		goto bad_include;

	for (p = s; (c = *p) && c != delim; ++p)
		/* nothing */;
	if (c == '\0')
		goto bad_include;

	len = p - s;
	if (delim == '"') {
		if (len >= FILENAME_MAX)
			goto too_long;
		strncpy(fname, s, len);
		fname[len] = '\0';
		if (!addinput(fname))
			goto not_found;
	} else {
		abort();
	}

	return p+1;

not_found:
	error("included file '%s' not found", fname);
too_long:
	error("file name in include too long");
bad_include:
	error("#include expects \"FILENAME\" or <FILENAME>");
}

static char *
preprocessor(char *p)
{
	char str[IDENTSIZ+1], *q;
	unsigned short n;
	static char **bp, *cmds[] = {
		"include",
		NULL
	};
	static char *(*funs[])(char *) = {
		include
	};

	while (isspace(*p))
		++p;
	if (*p != '#')
		return p;
	for (++p; isspace(*p); ++p)
		/* nothing */;
	for (q = p; isalpha(*q); ++q)
		/* nothing */;
	n = q - p;
	while (isspace(*q))
		++q;
	for (bp = cmds; *bp; ++bp) {
		if (strncmp(*bp, p, n))
			continue;
		q = (*funs[bp - cmds])(q);
		while (isspace(*q++))
			/* nothing */;
		if (*q != '\0')
			error("trailing characters after preprocessor directive");
		return NULL;
	}
	error("incorrect preprocessor directive");
}

static int
readchar(void)
{
	int c;
	FILE *fp = input->fp;

repeat:
	if ((c = getc(fp)) == '\\') {
		if ((c = getc(fp)) == '\n')
			goto repeat;
		ungetc(c, fp);
		c = '\\';
	}
	return c;
}

static void
readline(void)
{
	static int comment, commentline;
	char *bp, *lim;
	int c;

	bp = input->line;
	lim = bp + INPUTSIZ-1;

	for (;;) {
		c = readchar();
	nextchar:
		if (c == EOF)
			break;
		if (comment) {
			if (c != '*')
				continue;
			if ((c = readchar()) != '/')
				goto nextchar;
			comment = 0;
			c = ' ';
		} else if (commentline) {
			if (c != '\n')
				continue;
			commentline = 0;
			c = ' ';
		}
		if (c == '\n')
			break;
		if (bp == lim)
			die("line %d too big in file '%s'",
			    input->nline, input->fname);
		if (c == '/') {
			if ((c = readchar()) == '*') {
				comment = 1;
				continue;
			} else if (c == '/') {
				commentline = 1;
				continue;
			}
			*bp++ = '/';
			goto nextchar;
		}
		*bp++ = c;
	}
	ungetc(c, input->fp);
	*bp = '\0';
}

static bool
fill(void)
{
	int c;
	char *p;
	FILE *fp;

repeat:
	if (!input)
		return 0;
	if (input->begin && *input->begin != '\0')
		return 1;

	fp = input->fp;
	if (!input->line)
		input->line = xmalloc(INPUTSIZ);

	while ((c = getc(fp)) != EOF && (c == '\n')) {
		if (++input->nline == 0)
			die("input file '%s' too long", input->fname);
	}
	if (c == EOF) {
		delinput();
		goto repeat;
	}
	ungetc(c, fp);
	readline();
	if ((p = preprocessor(input->line)) == NULL)
		goto repeat;
	input->begin = input->p = p;
	return 1;
}

static void
tok2str(void)
{
	size_t len;

	if ((len = input->p - input->begin) > IDENTSIZ)
		error("token too big");
	strncpy(yytext, input->begin, len);
	yytext[len] = '\0';
	fprintf(stderr ,"%s\n", yytext);
	input->begin = input->p;
}

static uint8_t
integer(char *s, char base)
{
	Type *tp;
	Symbol *sym;
	uint8_t size, sign;
	long v;

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
	sym = install("", NS_IDEN);
	sym->type = tp;
	v = strtol(s, NULL, base);
	if (tp == inttype)
		sym->u.i = v;
	yylval.sym = sym;
	return CONSTANT;
}

static char *
digits(uint8_t base)
{
	char c, *p;

	for (p = input->p; c = *p; ++p) {
		switch (base) {
		case 8:
			if (c > '7' || c < '0')
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

static uint8_t
number(void)
{
	int ch;
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

	++input->p;
	switch (*input->p++) {
	case '\\': return '\\';
	case 'a':  return '\a';
	case 'f':  return '\f';
	case 'n':  return '\n';
	case 'r':  return '\r';
	case 't':  return '\t';
	case 'v':  return '\v';
	case '\'': return '\\';
	case '"':  return'"';
	case '?':  return '?';
	case 'u':  base = 10; break;
	case 'x':  base = 16; break;
	case '0':  base = 8; break;
	default:
		warn("unknown escape sequence");
		return ' ';
	}
	errno = 0;
	c = strtoul(input->p, &input->p, base);
	if (errno || c > 255)
		warn("character constant out of range");
	return c;
}

static uint8_t
character(void)
{
	static char c;
	Symbol *sym;

	if ((c = *++input->p) == '\\')
		c = escape();
	if (*input->p != '\'')
		error("invalid character constant");
	++input->p;

	sym = install("", NS_IDEN);
	sym->u.i = c;
	sym->type = inttype;
	yylval.sym = sym;
	return CONSTANT;
}

static uint8_t
string(void)
{
	static char buf[STRINGSIZ+1];
	Symbol *sym;
	char *bp = buf, c;

	assert(STRINGSIZ <= INPUTSIZ);
	for (++input->p; (c = *input->p) != '\0'; ++input->p) {
		if (c == '"')
			break;
		if (c == '\\')
			c = escape();
		*bp++ = c;
	}

	if (c == '\0')
		error("missing terminating '\"' character");
	*bp = '\0';
	sym = install("", NS_IDEN);
	sym->u.s = xstrdup(buf);
	sym->type = mktype(chartype, ARY, (bp - buf) + 1, NULL);
	yylval.sym = sym;
	return CONSTANT;
}

static uint8_t
iden(void)
{
	char c, *p;
	Symbol *sym;

	for (p = input->p; isalpha(*p); ++p)
		/* nothing */;
	input->p = p;
	tok2str();
	sym = yylval.sym = lookup(yytext, lex_ns);
	if (!sym || sym->token == IDEN)
		return IDEN;
	yylval.token = sym->u.token;
	return sym->token;
}

static uint8_t
follow(int expect, int ifyes, int ifno)
{
	if (*input->p++)
		return ifyes;
	--input->p;
	return ifno;
}

static uint8_t
minus(void)
{
	switch (*input->p++) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default: --input->p; return '-';
	}
}

static uint8_t
plus(void)
{
	switch (*input->p++) {
	case '+': return INC;
	case '=': return ADD_EQ;
	default: --input->p; return '+';
	}
}

static uint8_t
relational(uint8_t op, uint8_t equal, uint8_t shift, uint8_t assig)
{
	char c;

	if ((c = *input->p++) == '=')
		return equal;
	if (c == op)
		return follow('=', assig, shift);
	--input->p;
	return op;
}

static uint8_t
logic(uint8_t op, uint8_t equal, uint8_t logic)
{
	char c;

	if ((c = *input->p++) == equal)
		return equal;
	if (c == op)
		return logic;
	--input->p;
	return op;
}

static uint8_t
dot(void)
{
	char c;

	if (c = *input->p != '.')
		return '.';
	if ((c = *++input->p) != '.')
		error("incorrect token '..'");
	++input->p;
	return ELLIPSIS;
}

static uint8_t
operator(void)
{
	uint8_t t;

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
	case '-': t = minus(); break;
	case '+': t = plus(); break;
	case '.': t = dot(); break;
	}
	tok2str();
	return t;
}

uint8_t
next(void)
{
	char c;

	if (!fill())
		return EOFTOK;

	while (isspace(*input->begin))
		++input->begin;
	c = *(input->p = input->begin);

	if (isalpha(c) || c == '_') {
		yytoken = iden();
	} else if (isdigit(c)) {
		yytoken = number();
	} else if (c == '"') {
		yytoken = string();
	} else if (c == '\'') {
		yytoken = character();
	} else {
		yytoken = operator();
	}
	return yytoken;
}

void
expect(uint8_t tok)
{
	if (yytoken != tok) {
		if (isgraph(tok))
			softerror("expected '%c' before '%s'", tok, yytext);
		else
			softerror("unexpected '%s'", yytext);
	} else {
		next();
	}
}

uint8_t
ahead(void)
{
	int c;

repeat:
	if (!input)
		return EOFTOK;
	while (isspace(c = *input->begin))
		;
	if (c == '\0') {
		fill();
		goto repeat;
	}
	return c;
}

void
setsafe(uint8_t type)
{
	safe = type;
}

void
discard(void)
{
	extern jmp_buf recover;
	char c;

	for (c = yytoken; ; c = *input->p++) {
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
		if (*input->p == '\0')
			fill();
		if (!input)
			break;
	}

	c = EOFTOK;
jump:
	yytoken = c;
	longjmp(recover, 1);
}
