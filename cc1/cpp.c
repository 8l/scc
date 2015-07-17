
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

#ifndef PREFIX
#define PREFIX "/usr/include/local"
#endif

static char *argp, *macroname;
static unsigned arglen;
static Symbol *symline, *symfile;
static unsigned char ifstatus[NR_COND];
static Type *charptype;

unsigned cppctx;
int disexpand;

static Symbol *
defmacro(char *s)
{
	Symbol *sym;

	strcpy(yytext, s);
	sym = lookup(NS_CPP);
	sym->flags |= ISDEFINED;
	return sym;
}

void
icpp(void)
{
	static char sdate[17], stime[14];
	struct tm *tm;
	time_t t;

	t = time(NULL);
	tm = localtime(&t);
	strftime(sdate, sizeof(sdate), "-1#\"%b %d %Y\"", tm);
	strftime(stime, sizeof(stime), "-1#\"%H:%M:%S\"", tm);

	defmacro("__STDC__")->u.s = "-1#1";
	defmacro("__DATE__")->u.s = sdate;
	defmacro("__TIME__")->u.s = stime;
	defmacro("__STDC_HOSTED__")->u.s = "-1#1";
	defmacro("__STDC_VERSION__")->u.s = "-1#199409L";
	symline = defmacro("__LINE__");
	symfile = defmacro("__FILE__");
}

static void
nextcpp(void)
{
	next();
	if (yytoken == EOFTOK)
		error("unterminated argument list invoking macro \"%s\"",
		      macroname);
	if (yylen + 1 > arglen)
		error("argument overflow invoking macro \"%s\"",
		      macroname);
	memcpy(argp, yytext, yylen);
	argp += yylen;
	*argp++ = ' ';
	arglen -= yylen + 1;
}

static void
paren(void)
{
	for (;;) {
		nextcpp();
		switch (yytoken) {
		case ')':
			return;
		case '(':
			paren();
			break;
		}
	}
}

static void
parameter(void)
{
	for (;;) {
		nextcpp();
		switch (yytoken) {
		case ')':
		case ',':
			argp -= 3;  /* remove " , "  or " ) "*/
			*argp++ = '\0';
			return;
		case '(':
			paren();
			break;
		}
	}
}

static int
parsepars(char *buffer, char **listp, int nargs)
{
	int n;

	if (nargs == -1)
		return -1;
	if (ahead() != '(' && nargs > 0)
		return 0;

	disexpand = 1;
	next();
	n = 0;
	argp = buffer;
	arglen = INPUTSIZ;
	if (yytoken != ')') {
		do {
			*listp++ = argp;
			parameter();
		} while (++n < NR_MACROARG && yytoken == ',');
	}
	if (yytoken != ')')
		error("incorrect macro function alike invocation");
	disexpand = 0;

	if (n == NR_MACROARG)
		error("too much parameters in macro \"%s\"", macroname);
	if (n != nargs) {
		error("macro \"%s\" passed %d arguments, but it takes %d",
		      macroname, n, nargs);
	}

	return 1;
}

static void
copymacro(char *bp, char *s, size_t bufsiz, char *arglist[])
{
	char prevc, c, *arg;

	for (prevc = '\0'; c = *s; prevc = c, ++s) {
		if (c != '@') {
			if (c == '#')
				continue;
			if (bufsiz-- == 0)
				goto expansion_too_long;
			*bp++ = c;
		} else {
			size_t size;

			if (prevc == '#')
				bufsiz -= 2;
			arg = arglist[atoi(++s)];
			size = strlen(arg);
			if (size > bufsiz)
				goto expansion_too_long;
			if (prevc == '#')
				*bp++ = '"';
			memcpy(bp, arg, size);
			bp += size;
			if (prevc == '#')
				*bp++ = '"';
			bufsiz -= size;
			s += 2;
		}
	}
	*bp = '\0';

	return;

expansion_too_long:
	error("expansion of macro \"%s\" is too long", macroname);
}

#define BUFSIZE ((INPUTSIZ > FILENAME_MAX+2) ? INPUTSIZ : FILENAME_MAX+2)
bool
expand(char *begin, Symbol *sym)
{
	size_t len;
	int n;
	char *s = sym->u.s;
	char *arglist[NR_MACROARG], arguments[INPUTSIZ], buffer[BUFSIZE];

	fprintf(stderr, "macro '%s':%s\n", sym->name, sym->u.s);
	if (sym == symfile) {
		sprintf(buffer, "\"%s\"", input->fname);
		goto print_subs;
	}
	if (sym == symline) {
		sprintf(buffer, "%d", input->line);
		goto print_subs;
	}

	macroname = sym->name;
	if (!parsepars(arguments, arglist, atoi(s)))
		return 0;
	for (n = 0; n < atoi(s); ++n)
		fprintf(stderr, "PAR%d:%s\n", n, arglist[n]);

	copymacro(buffer, s+3, INPUTSIZ-1, arglist);

print_subs:
	fprintf(stderr, "macro '%s' expanded to :'%s'\n", macroname, buffer);
	len = strlen(buffer);

	if (begin - input->line + len >= LINESIZ-1)
		error("macro expansion too long");

	/* cut macro invocation */
	memmove(begin, input->p, input->p - begin);

	/* paste macro expansion */
	memmove(begin + len, begin, len);
	memcpy(begin, buffer, len);

	input->p = input->begin = begin;

	return 1;
}
#undef BUFSIZE

static int
getpars(Symbol *args[NR_MACROARG])
{
	int n = -1;
	char *err;

	if (!accept('('))
		return n;
	++n;
	if (accept(')'))
		return n;

	do {
		if (n == NR_MACROARG) {
			err = "too much parameters in macro";
			goto popctx_and_error;
		}
		if (yytoken != IDEN) {
			err = "macro arguments must be identifiers";
			goto popctx_and_error;
		}
		args[n++] = yylval.sym;
		next();
	} while (accept(','));
	expect(')');

	return n;

popctx_and_error:
	popctx();
	error(err);
}

static void
getdefs(Symbol *args[NR_MACROARG], int nargs, char *bp, size_t bufsiz)
{
	Symbol **argp;
	char *err;
	size_t len;
	int prevc = 0, ispar;

	for (;;) {
		ispar = 0;
		if (yytoken == IDEN) {
			for (argp = args; argp < &args[nargs]; ++argp) {
				if (*argp == yylval.sym)
					break;
			}
			if (argp != &args[nargs]) {
				sprintf(yytext, "@%02d@", argp - args);
				ispar = 1;
			}
		}
		if (prevc == '#' && !ispar)
			goto bad_stringer;
		if (yytoken == EOFTOK)
			break;

		if ((len = strlen(yytext)) >= bufsiz) {
			err = "too long macro";
			goto popctx_and_error;
		}
		memcpy(bp, yytext, len);
		bp += len;
		bufsiz -= len;
		if ((prevc = yytoken) != '#') {
			bufsiz;
			*bp++ = ' ';
		}
		next();
	}
	*bp = '\0';
	return;

bad_stringer:
	err = "'#' is not followed by a macro parameter";
popctx_and_error:
	popctx();
	error(err);
}

static void
define(void)
{
	Symbol *sym,*args[NR_MACROARG];
	char buff[LINESIZ+1];
	int n;

	if (cppoff)
		return;
	if (yytoken != IDEN)
		error("macro names must be identifiers");
	sym = yylval.sym;
	if ((sym->flags & ISDEFINED) && sym->ns == NS_CPP) {
		warn("'%s' redefined", yytext);
		free(sym->u.s);
	} else if (sym->ns != NS_CPP) {
		sym = lookup(NS_CPP);
	}
	sym->flags |= ISDEFINED;

	pushctx();

	next();
	n = getpars(args);
	sprintf(buff, "%02d#", n);
	getdefs(args, n, buff+3, LINESIZ-3);
	sym->u.s = xstrdup(buff);
	fprintf(stderr, "Defining macro '%s'='%s'\n", sym->name, buff);

	popctx();
}

static void
include(void)
{
	char **bp, *p, file[FILENAME_MAX], path[FILENAME_MAX];
	static char *sysinclude[] = {
		PREFIX"/include/",
		PREFIX"/local/include/",
		NULL
	};
	size_t filelen, dirlen;

	if (cppoff)
		return;
	switch (*yytext) {
	case '<':
		if ((p = strchr(input->begin, '>')) == NULL)
			goto bad_include;
		*p = '\0';
		if (p - input->begin >= FILENAME_MAX)
			goto too_long;
		strcpy(file, input->begin);
		input->begin = input->p = p+1;
		next();
		break;
	case '"':
		if ((p = strchr(yytext + 1, '"')) == NULL)
			goto bad_include;
		*p = '\0';
		if (p - yytext + 1 >= FILENAME_MAX)
			goto too_long;
		strcpy(file, yytext + 1);
		next();
		if (addinput(file))
			return;
		break;
	default:
		goto bad_include;
	}

	filelen = strlen(file);
	for (bp = sysinclude; *bp; ++bp) {
		dirlen = strlen(*bp);
		if (dirlen + filelen > FILENAME_MAX-1)
			continue;
		memcpy(path, *bp, dirlen);
		memcpy(path+dirlen, file, filelen);
		if (addinput(path))
			break;
	}
	if (*bp)
		error("included file '%s' not found", file);

	return;

bad_include:
	error("#include expects \"FILENAME\" or <FILENAME>");
too_long:
	error("#include FILENAME too long");
}

static void
line(void)
{
	char *file, *p;
	Type *tp;
	long n;

	if (cppoff)
		return;
	if ((n = strtol(input->p, &input->p, 10)) <= 0 || n > USHRT_MAX)
		error("first parameter of #line is not a positive integer");

	if (yytoken != CONSTANT || yylval.sym->type != inttype)
		error("first parameter of #line is not a positive integer");

	input->nline = yylval.sym->u.i;
	next();
	if (yytoken == EOFTOK)
		return;

	tp = yylval.sym->type;
	if (yytoken != CONSTANT || tp->op != ARY && tp->type != chartype)
		error("second parameter of #line is not a valid filename");
	free(input->fname);
	input->fname = xstrdup(yylval.sym->u.s);
}

static void
pragma(void)
{
	if (cppoff)
		return;
	/* TODO: discard input */
}

static void
usererr(void)
{
	if (cppoff)
		return;
	printerr("#error %s", input->p);
	/* TODO: discard input */
}

static void
ifclause(int isdef)
{
	Symbol *sym;
	unsigned n;

	if (cppctx == NR_COND-1)
		error("too much nesting levels of conditional inclusion");
	n = cppctx++;
	if (yytoken != IDEN) {
		error("no macro name given in #%s directive",
		      (isdef) ? "ifdef" : "ifndef");
	}

	sym = lookup(NS_CPP);
	next();
	if (!(ifstatus[n] = (sym->flags & ISDEFINED) != 0 == isdef))
		++cppoff;
}

static void
ifdef(void)
{
	ifclause(1);
}

static void
ifndef(void)
{
	ifclause(0);
}

static void
endif(void)
{
	if (cppctx == 0)
		error("#endif without #if");

	if (!ifstatus[--cppctx])
		--cppoff;
}

static void
elseclause(void)
{
	struct ifstatus *ip;

	if (cppctx == 0)
		error("#else without #ifdef/ifndef");

	cppoff += (ifstatus[cppctx-1] ^= 1) ? -1 : 1;
}

static void
undef(void)
{
	Symbol *sym;

	if (cppoff)
		return;
	if (yytoken != IDEN) {
		error("no macro name given in #undef directive");
		return;
	}
	sym = lookup(NS_CPP);
	sym->flags &= ~ISDEFINED;
}

bool
cpp(void)
{
	static struct {
		uint8_t tok;
		void (*fun)(void);
	} *bp, clauses [] = {
		{DEFINE, define},
		{INCLUDE, include},
		{LINE, line},
		{IFDEF, ifdef},
		{IFNDEF, ifndef},
		{ELSE, elseclause},
		{UNDEF, undef},
		{PRAGMA, pragma},
		{ERROR, usererr},
		{0, NULL}
	};

	if (*input->p != '#')
		return 0;
	++input->p;
	lexmode = CPPMODE;
	setnamespace(NS_CPPCLAUSES);
	next();
	for (bp = clauses; bp->tok && bp->tok != yytoken; ++bp)
		/* nothing */;
	if (!bp->tok)
		error("incorrect preprocessor directive");
	next();
	(*bp->fun)();

	if (yytoken != EOFTOK && !cppoff)
		error("trailing characters after preprocessor directive");
	lexmode = CCMODE;
	return 1;
}
