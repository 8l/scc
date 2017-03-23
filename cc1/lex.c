/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc1/lex.c";
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstd.h>
#include "../inc/cc.h"
#include "cc1.h"

unsigned yytoken;
struct yystype yylval;
char yytext[STRINGSIZ+3];
unsigned short yylen;
int lexmode = CCMODE;
unsigned lineno;
char filenam[FILENAME_MAX];

int namespace = NS_IDEN;
static int safe;
Input *input;

void
ilex(void)
{
	static struct keyword keys[] = {
		{"auto", SCLASS, AUTO},
		{"break", BREAK, BREAK},
		{"_Bool", TYPE, BOOL},
		{"__builtin_va_list", TYPE, VA_LIST},
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
	keywords(keys, NS_KEYWORD);
}

int
setloc(char *fname, unsigned line)
{
	size_t len;

	if ((len = strlen(fname)) >= FILENAME_MAX)
		die("file name too long: '%s'", fname);
	memcpy(filenam, fname, len);
	filenam[len] = '\0';

	free(input->filenam);
	input->filenam = xstrdup(fname);
	lineno = input->lineno = line;
	return 1;
}

int
addinput(char *fname, Symbol *hide, char *buffer)
{
	FILE *fp;
	char *extp;
	unsigned flags;
	int infileln;
	Input *newip, *curip = input;

	if (hide) {
		/* this is a macro expansion */
		fp = NULL;
		if (hide->hide == UCHAR_MAX)
			die("Too many macro expansions");
		++hide->hide;
		flags = IMACRO;
	} else  if (fname) {
		/* a new file */
		if ((fp = fopen(fname, "r")) == NULL)
			return 0;
		flags = IFILE;
		if (curip && onlyheader) {
			infileln = strlen(infile);
			if (extp = strrchr(infile, '.'))
				infileln -= strlen(extp);
			printf("%.*s.o: %s %s\n",
			       infileln, infile, infile, fname);
		}
	} else {
		/* reading from stdin */
		fp = stdin;
		fname = "<stdin>";
		flags = ISTDIN;
	}

	newip = xmalloc(sizeof(*newip));

	if (!buffer) {
		buffer = xmalloc(INPUTSIZ);
		buffer[0] = '\0';
	}

	if (curip)
		curip->lineno = lineno;

	newip->p = newip->begin = newip->line = buffer;
	newip->filenam = NULL;
	newip->lineno = 0;
	newip->next = curip;
	newip->fp = fp;
	newip->hide = hide;
	newip->flags = flags;
	input = newip;

	return setloc(fname, (curip) ? curip->lineno : newip->lineno);
}

void
delinput(void)
{
	Input *ip = input;
	Symbol *hide = ip->hide;

	switch (ip->flags & ITYPE) {
	case IFILE:
		if (fclose(ip->fp))
			die("error: failed to read from input file '%s'",
			    ip->filenam);
		break;
	case IMACRO:
		assert(hide->hide == 1);
		--hide->hide;
		break;
	}
	input = ip->next;
	free(ip->filenam);
	free(ip->line);
	if (input) {
		lineno = input->lineno;
		strcpy(filenam, input->filenam);
	}
}

static void
newline(void)
{
	if (++lineno == 0)
		die("error: input file '%s' too long", filenam);
}

/*
 * Read the next character from the input file, counting number of lines
 * and joining lines escaped with \
 */
static int
readchar(void)
{
	FILE *fp = input->fp;
	int c;

repeat:
	switch (c = getc(fp)) {
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

/*
 * discard a C comment. This function is only called from readline
 * because it is impossible to have a comment in a macro, because
 * comments are always discarded before processing any cpp directive
 */
static void
comment(int type)
{
	int c;

repeat:
	while ((c = readchar()) != EOF && c != type)
		/* nothing */;

	if (c == EOF) {
		errorp("unterminated comment");
		return;
	}

	if (type == '*' && (c = readchar()) != '/')
		goto repeat;
}

/*
 * readline is used to read a full logic line from a file.
 * It discards comments and check that the line fits in
 * the input buffer
 */
static int
readline(void)
{
	char *bp, *lim;
	int c, peekc = 0;

	if (feof(input->fp)) {
		input->flags |= IEOF;
		return 0;
	}

	*input->line = '\0';
	lim = &input->line[INPUTSIZ-1];
	for (bp = input->line; bp < lim-1; *bp++ = c) {
		c = (peekc) ? peekc : readchar();
		peekc = 0;
		if (c == '\n' || c == EOF)
			break;
		if (c != '/')
			continue;

		/* check for /* or // */
		peekc = readchar();
		if (peekc != '*' && peekc != '/')
			continue;
		comment((peekc == '/') ? '\n' : '*');
		peekc = 0;
		c = ' ';
	}

	input->begin = input->p = input->line;
	if (bp == lim-1) {
		errorp("line too long");
		--bp;
	}
	*bp++ = '\n';
	*bp = '\0';

	return 1;
}

/*
 * moreinput gets more bytes to be passed to the lexer.
 * It can take more bytes from macro expansions or
 * directly reading from files. When a cpp directive
 * is processed the line is discarded because it must not
 * be passed to the lexer
 */
static int
moreinput(void)
{
	int wasexpand = 0;

repeat:
	if (!input)
		return 0;

	if (*input->p == '\0') {
		if ((input->flags&ITYPE) == IMACRO) {
			wasexpand = 1;
			input->flags |= IEOF;
		}
		if (input->flags & IEOF) {
			delinput();
			goto repeat;
		}
		if (!readline() || cpp()) {
			*input->p = '\0';
			goto repeat;
		}
	}

	if (onlycpp && !wasexpand)
		ppragmaln();
	return 1;
}

static void
tok2str(void)
{
	if ((yylen = input->p - input->begin) > INTIDENTSIZ)
		error("token too big");
	memcpy(yytext, input->begin, yylen);
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
	sym = newsym(NS_IDEN, NULL);
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
	char c;
	Symbol *sym;

	if ((c = *++input->p) == '\\')
		c = escape();
	else
		c = *input->p;
	++input->p;
	if (*input->p != '\'')
		errorp("invalid character constant");
	else
		++input->p;

	sym = newsym(NS_IDEN, NULL);
	sym->u.i = c;
	sym->type = inttype;
	yylval.sym = sym;
	tok2str();
	return CONSTANT;
}

static unsigned
string(void)
{
	char *bp = yytext, c;

	*bp++ = '"';
	for (++input->p; (c = *input->p) != '"'; ++input->p) {
		if (c == '\0') {
			errorp("missing terminating '\"' character");
			break;
		}
		if (c == '\\')
			c = escape();
		if (bp == &yytext[STRINGSIZ+1]) {
			/* TODO: proper error handling here */
			error("string too long");
		}
		*bp++ = c;
	}

	input->begin = ++input->p;
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
	if ((sym = lookup(NS_CPP, yytext, NOALLOC)) != NULL) {
		if (!disexpand && !sym->hide && expand(begin, sym))
			return next();
	}
	sym = lookup(namespace, yytext, ALLOC);
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

/*
 * skip all the spaces until the next token. When we are in
 * CPPMODE \n is not considered a whitespace
 */
static int
skipspaces(void)
{
	int c;

	for (;;) {
		switch (c = *input->p) {
		case '\n':
			if (lexmode == CPPMODE)
				goto return_byte;
			++input->p;
		case '\0':
			if (!moreinput())
				return EOF;
			break;
		case ' ':
		case '\t':
		case '\v':
		case '\r':
		case '\f':
			++input->p;
			break;
		default:
			goto return_byte;
		}
	}

return_byte:
	input->begin = input->p;
	return c;
}

unsigned
next(void)
{
	int c;

	if ((c = skipspaces()) == EOF)
		yytoken = EOFTOK;
	else if (isalpha(c) || c == '_')
		yytoken = iden();
	else if (isdigit(c))
		yytoken = number();
	else if (c == '"')
		yytoken = string();
	else if (c == '\'')
		yytoken = character();
	else
		yytoken = operator();

	if (yytoken == EOF) {
		strcpy(yytext, "<EOF>");
		if (cppctx)
			errorp("#endif expected");
	}

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
