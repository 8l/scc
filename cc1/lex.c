
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

typedef struct input Input;

struct input {
	char *fname;
	unsigned short nline;
	FILE *fp;
	char *line, *begin, *p;
	Symbol *macro;
	struct input *next;
};

unsigned yytoken;
struct yystype yylval;
char yytext[IDENTSIZ + 1];
unsigned short yylen;
int cppoff;

static unsigned lex_ns = NS_IDEN;
static int safe, eof;
static Input *input;

static void
allocinput(char *fname, FILE *fp, char *buff)
{
	Input *ip;

	ip = xmalloc(sizeof(Input));
	ip->fname = fname;
	ip->next = input;
	ip->macro = NULL;
	ip->begin = ip->line = buff;
	ip->nline = (fp) ? 0 : input->nline;
	ip->fp = fp;
	input = ip;
}

void
ilex(char *fname)
{
	FILE *fp;

	/*
	 * we can use static file names because this Input is not going
	 * to be freed ever
	 */
	if (!fname) {
		fp = stdin;
		fname = "<stdin>";
	} else {
		if ((fp = fopen(fname, "r")) == NULL)
			die("error opening output:%s", strerror(errno));
		fname = fname;
	}
	allocinput(fname, fp, xmalloc(INPUTSIZ));
	*input->begin = '\0';
}

bool
addinput(char *fname, Symbol *sym, char *str)
{
	FILE *fp;
	char flags = 0;

	if (fname) {
		/*
		 * this call comes from an include clause, so we reuse
		 * the buffer from the calling Input
		 */
		if ((fp = fopen(fname, "r")) == NULL)
			return 0;
		fname = xstrdup(fname);
		str = input->line;
		*str = '\0';
	} else {
		/*
		 * This call comes from a macro expansion, so we have
		 * to duplicate the input string because it is the
		 * expansion of the macro in a temporal buffer
		 */
		fname = input->fname;
		fp = NULL;
		str = xstrdup(str);
	}
	allocinput(fname, fp, str);
	input->macro = sym;
	return 1;
}

static void
delinput(void)
{
	Input *ip;

repeat:
	if (input->fp) {
		/* include input */
		if (fclose(input->fp))
			die("error reading from input file '%s'", input->fname);
		if (!input->next) {
			eof = 1;
			return;
		}
		free(input->fname);
	} else {
		/* macro input */
		free(input->line);
	}
	ip = input;
	input = input->next;
	free(ip);

	if (*input->begin != '\0')
		return;
	if (!input->fp)
		goto repeat;
}

void
setfname(char *name)
{
	free(input->fname);
	input->fname = xstrdup(name);
}

char *
getfname(void)
{
	return input->fname;
}

void
setfline(unsigned short line)
{
	input->nline = line;
}

unsigned short
getfline(void)
{
	return input->nline;
}

static char
readchar(void)
{
	int c;
	FILE *fp;

repeat:
	if (feof(input->fp))
		delinput();
	if (eof)
		return '\0';
	fp = input->fp;

	if ((c = getc(fp)) == '\\') {
		if ((c = getc(fp)) == '\n')
			goto repeat;
		ungetc(c, fp);
		c = '\\';
	} else if (c == EOF) {
		c = '\n';
	} else if (c == '\n' && ++input->nline == 0) {
		die("error:input file '%s' too long", getfname());
	}
	return c;
}

static void
comment(char type)
{
	if (type == '*') {
		while (!eof) {
			while (readchar() !=  '*' && !eof)
				/* nothing */
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

static bool
readline(void)
{
	char *bp, *lim;
	char c, peekc = 0;

	if (eof)
		return 0;
	lim = &input->line[INPUTSIZ-1];
	for (bp = input->line; bp != lim; *bp++ = c) {
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
			break;
		}
	}

	if (bp == lim)
		error("line %u too big in file '%s'", getfline(), getfname());
	*bp = '\0';
	return 1;
}

bool
moreinput(void)
{
	char *p;

repeat:
	if (!input->fp)
		delinput();
	if (*input->begin)
		return 1;
	if (!readline())
		return 0;
	p = input->line;
	while (isspace(*p))
		++p;
	if (*p == '\0' || cpp(p) || cppoff) {
		*input->begin = '\0';
		goto repeat;
	}

	input->p = input->begin = p;
	return 1;
}

static void
tok2str(void)
{
	if ((yylen = input->p - input->begin) > IDENTSIZ)
		error("token too big");
	strncpy(yytext, input->begin, yylen);
	yytext[yylen] = '\0';
	fprintf(stderr ,"%s\n", yytext);
	input->begin = input->p;
}

static unsigned
integer(char *s, char base)
{
	Type *tp;
	Symbol *sym;
	unsigned size, sign;
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
	sym = newsym(NS_IDEN);
	sym->type = tp;
	v = strtol(s, NULL, base);
	if (tp == inttype)
		sym->u.i = v;
	yylval.sym = sym;
	return CONSTANT;
}

static char *
digits(unsigned base)
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

static unsigned
character(void)
{
	static char c;
	Symbol *sym;

	if ((c = *++input->p) == '\\')
		c = escape();
	if (*input->p != '\'')
		error("invalid character constant");
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
	char buf[STRINGSIZ+1];
	Symbol *sym;
	char *bp = buf, c;

repeat:
	for (++input->p; (c = *input->p) != '\0' && c != '"'; ++input->p) {
		if (c == '\\')
			c = escape();
		if (bp == &buf[STRINGSIZ])
			error("string too long");
		*bp++ = c;
	}

	if (c == '\0')
		error("missing terminating '\"' character");
	input->begin = input->p + 1;

	if (ahead() == '"')
		goto repeat;
	*bp = '\0';
	fprintf(stderr, "\"%s\"\n", buf);
	sym = newsym(NS_IDEN);
	sym->u.s = xstrdup(buf);
	sym->type = mktype(chartype, ARY, (bp - buf) + 1, NULL);
	yylval.sym = sym;
	return CONSTANT;
}

static unsigned
iden(void)
{
	char *p, *t, c;

	for (p = input->p; isalnum(*p) || *p == '_'; ++p)
		/* nothing */;
	input->p = p;
	tok2str();
	yylval.sym = lookup(lex_ns);
	if (yylval.sym->ns == NS_CPP) {
		if (yylval.sym != input->macro && expand(yylval.sym))
			return 0;
		/*
		 * it is not a correct macro call, so try to find
		 * another definition. This is going to be expensive
		 * but I think it is not going to be a common case.
		 */
		yylval.sym = nextsym(yylval.sym, lex_ns);
	}
	if (yylval.sym->token != IDEN)
		yylval.token = yylval.sym->u.token;
	return yylval.sym->token;
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

	if ((c = *input->p++) == equal)
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

	if (c = *input->p != '.')
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
	case '-': t = minus(); break;
	case '+': t = plus(); break;
	case '.': t = dot(); break;
	}
	tok2str();
	return t;
}

/* TODO: Ensure that lex_ns is NS_IDEN after a recovery */
void
setnamespace(int ns)
{
	lex_ns = ns;
}

static void
skipspaces(void)
{
	char *p;

repeat:
	for (p = input->begin; isspace(*p); ++p)
		/* nothing */;
	if (*p == '\0') {
		if (!moreinput())
			return;
		goto repeat;
	}
	input->begin = input->p = p;
}

unsigned
next(void)
{
	char c;

repeat:
	skipspaces();
	if (eof) {
		if (cppctx)
			error("#endif expected");
		strcpy(yytext, "<EOF>");
		return yytoken = EOFTOK;
	}

	c = *input->begin;
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

	if (!yytoken)
		goto repeat;

	lex_ns = NS_IDEN;
	return yytoken;
}

void
expect(unsigned tok)
{
	if (yytoken != tok) {
		if (isgraph(tok))
			printerr("expected '%c' before '%s'", tok, yytext);
		else
			printerr("unexpected '%s'", yytext);
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
			exit(-1);
	}
jump:
	yytoken = c;
	longjmp(recover, 1);
}
