
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#define INPUTSIZ 128

typedef struct input Input;

struct input {
	char *fname;
	unsigned short nline;
	int8_t cnt;
	FILE *fp;
	char *line, *ptr;
	struct input *next;
};

#define nextchar() ((--input->cnt >= 0) ? \
                       (unsigned char) *input->ptr++ : readline())

uint8_t lex_ns = NS_IDEN;

uint8_t yytoken;
struct yystype yylval;
char yytext[IDENTSIZ + 1];
static uint8_t safe;

static Input *input;

void
addinput(char *fname)
{
	Input *ip, *oip = input;

	ip = xmalloc(sizeof(Input));
	ip->next = input;
	ip->line = NULL;
	ip->cnt = 0;
	ip->nline = 1;
	input = ip;

	if (fname) {
		if ((ip->fp = fopen(fname, "r")) == NULL)
			die("file '%s' not found", fname);
		ip->fname = xstrdup(fname);
		next();
	} else if (!oip) {
		ip->fp = stdin;
		ip->fname = "(stdin)";
		next();
	} else {
		ip->fname = oip->fname;
		ip->fp = NULL;
		ip->nline = input->nline;
	}
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

static void
newline(void)
{
	if (++input->nline == 0)
		die("input file '%s' too long", input->fname);
}

static int
readline(void)
{
	char *bp, *ptr;
	uint8_t n;
	int c;
	FILE *fp = input->fp;

	if (!input->line)
		input->line = xmalloc(INPUTSIZ);
	ptr = input->line;

	while ((c = getc(fp)) != EOF && isspace(c)) {
		if (c == '\n')
			newline();
	}
	if (c == EOF)
		return EOF;
	ungetc(c, fp);

	for (bp = ptr; (c = getc(fp)) != EOF && c != '\n'; ) {
		if (c == '\\') {
			if ((c = getc(fp)) == '\n')
				continue;
			ungetc(c, fp);
			c = '\\';
		}
		if (bp == &ptr[INPUTSIZ])
			die("line %d too big in file '%s'",
			    input->nline, input->fname);
		*bp++ =  c;
	}
	*bp = ' ';
	input->cnt = bp - ptr;
	input->ptr = ptr;
	return *input->ptr++;
}

static int
backchar(int c)
{
	++input->cnt;
	return *--input->ptr = c;
}

static uint8_t
integer(char *s, char base)
{
	static Type *tp;
	static Symbol *sym;
	static char ch, size, sign;
	static long v;

	size = sign = 0;
type:
	switch (ch = toupper(nextchar())) {
	case 'L':
		if (size == LLONG)
			goto wrong_type;
		size = (size == LONG) ? LLONG : LONG;
		goto type;
	case 'U':
		if (sign == UNSIGNED)
			goto wrong_type;
		goto type;
	default:
		backchar(ch);
		tp = ctype(INT, sign, size);
		break;
	wrong_type:
		error("invalid suffix in integer constant");
	}

	sym = install("", NS_IDEN);
	sym->type = tp;
	v = strtol(yytext, NULL, base);
	if (tp == inttype)
		sym->u.i = v;
	yylval.sym = sym;
	return CONSTANT;
}

static char *
digits(uint8_t base)
{
	char ch, *bp;

	for (bp = yytext ; bp < &yytext[IDENTSIZ]; *bp++ = ch) {
		ch = nextchar();
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
end:
	if (bp == &yytext[IDENTSIZ])
		error("number too long %s", yytext);
	*bp = '\0';
	backchar(ch);
	return yytext;
}

static uint8_t
number(void)
{
	int ch;
	static char base;

	if ((ch = nextchar()) == '0') {
		if (toupper(ch = nextchar()) == 'X') {
			base = 16;
		} else {
			base = 8;
			backchar(ch);
		}
	} else {
		base = 10;
		backchar(ch);
	}

	return integer(digits(base), base);
}

static char *
escape(char *s)
{
	uint8_t base;
	int c;

repeat:
	switch (nextchar()) {
	case '\\': c = '\''; break;
	case 'a':  c = '\a'; break;
	case 'f':  c = '\f'; break;
	case 'n':  c = '\n'; break;
	case 'r':  c = '\r'; break;
	case 't':  c = '\t'; break;
	case 'v':  c = '\v'; break;
	case '\'': c = '\\'; break;
	case '"':  c ='"';   break;
	case '?':  c = '?';  break;
	case 'u': /* TODO: */
	case 'x':
		base = 16;
		goto number;
	case '0':
		base = 8;
	number:
		if ((c = atoi(digits(base))) > 255)
			warn("character constant out of range");
		break;
	case '\n':
		newline();
		if ((c = nextchar()) == '\\')
			goto repeat;
		break;
	default:
		warn("unknown escape sequence");
		return s;
	}

	*s = c;
	return ++s;
}

static uint8_t
character(void)
{
	static char c;
	Symbol *sym;

	nextchar();   /* discard the initial ' */
	c = nextchar();
	if (c == '\\')
		escape(&c);
	if (nextchar() != '\'')
		error("invalid character constant");
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
	char *bp;
	int c;
	static Symbol *sym;

	nextchar(); /* discard the initial " */

	for (bp = buf; bp < &buf[STRINGSIZ]; ) {
		switch (c = nextchar()) {
		case EOF:
			error("found EOF while parsing");
		case '"':
			goto end_string;
		case '\\':
			bp = escape(bp);
			break;
		default:
			*bp++ = c;
		}
	}

end_string:
	if (bp == &buf[IDENTSIZ])
		error("string too long");
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
	char *bp;
	int c;
	Symbol *sym;

	for (bp = yytext; bp < &yytext[IDENTSIZ]; *bp++ = c) {
		if (!isalnum(c = nextchar()) && c != '_')
			break;
	}
	if (bp == &yytext[IDENTSIZ])
		error("identifier too long %s", yytext);
	*bp = '\0';
	backchar(c);

	sym = yylval.sym = lookup(yytext, lex_ns);
	if (!sym || sym->token == IDEN)
		return IDEN;
	yylval.token = sym->u.token;
	return sym->token;
}

static uint8_t
follow(int expect, int ifyes, int ifno)
{
	int c = nextchar();

	if (c == expect) {
		yytext[1] = c;
		yytext[2] = 0;
		return ifyes;
	}
	backchar(c);
	return ifno;
}

static uint8_t
minus(void)
{
	int c = nextchar();

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '-': return DEC;
	case '>': return INDIR;
	case '=': return SUB_EQ;
	default:
		yytext[1] = '\0';
		backchar(c);
		return '-';
	}
}

static uint8_t
plus(void)
{
	int c = nextchar();

	yytext[1] = c;
	yytext[2] = '\0';
	switch (c) {
	case '+': return INC;
	case '=': return ADD_EQ;
	default:
		yytext[1] = '\0';
		backchar(c);
		return '+';
	}
}

static uint8_t
relational(uint8_t op, uint8_t equal, uint8_t shift, uint8_t assig)
{
	int c = nextchar();

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return follow('=', assig, shift);
	backchar(c);
	yytext[1] = '\0';
	return op;
}

static uint8_t
logic(uint8_t op, uint8_t equal, uint8_t logic)
{
	int c = nextchar();

	yytext[1] = c;
	yytext[2] = '\0';

	if (c == '=')
		return equal;
	if (c == op)
		return logic;
	backchar(c);
	yytext[1] = '\0';
	return op;
}

static uint8_t
dot(void)
{
	int c;

	if ((c = nextchar()) != '.') {
		backchar(c);
		return '.';
	} else if ((c = nextchar()) != '.') {
		error("incorrect token '%s'", yytext);
	} else {
		yytext[2] = yytext[1] = '.';
		yytext[3] = '\0';
		return ELLIPSIS;
	}
}

static uint8_t
operator(void)
{
	uint8_t c = nextchar();

	yytext[0] = c;
	yytext[1] = '\0';
	switch (c) {
	case '<': return relational('<', LE, SHL, SHL_EQ);
	case '>': return relational('>', GE, SHR, SHR_EQ);
	case '&': return logic('&', AND_EQ, AND);
	case '|': return logic('|', OR_EQ, OR);
	case '=': return follow('=', EQ, '=');
	case '^': return follow('=', XOR_EQ, '^');
	case '*': return follow('=', MUL_EQ, '*');
	case '/': return follow('=', DIV_EQ, '/');
	case '!': return follow('=', NE, '!');
	case '-': return minus();
	case '+': return plus();
	case '.': return dot();
	default: return c;
	}
}

static int
skipspaces(void)
{

	int c;

	while (isspace(c = nextchar())) {
		if (c == '\n')
			newline();
	}
	return c;
}

uint8_t
next(void)
{
	int c;

	backchar(c = skipspaces());

	if (isalpha(c) || c == '_') {
		yytoken = iden();
	} else if (isdigit(c)) {
		yytoken = number();
	} else if (c == '"') {
		yytoken = string();
	} else if (c == '\'') {
		yytoken = character();
	} else if (c == EOF) {
		strcpy(yytext, "EOF");
		yytoken = EOFTOK;
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

	backchar(c = skipspaces());

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
	int c;

	c = yytoken;
	do {
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
	} while ((c = nextchar()) != EOF);

	c = EOFTOK;
jump:
	yytoken = c;
	longjmp(recover, 1);
}
