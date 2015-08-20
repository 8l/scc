
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

unsigned cppctx;
int disexpand;

static Symbol *
defmacro(char *s)
{
	Symbol *sym;

	strcpy(yytext, s);
	sym = lookup(NS_CPP);
	sym->flags |= ISDECLARED;
	return sym;
}

void
icpp(void)
{
	static char sdate[17], stime[14];
	struct tm *tm;
	time_t t;
	char **bp, *list[] = {
		"__STDC__",
		"__STDC_HOSTED__",
		"__SCC__",
		NULL
	};

	t = time(NULL);
	tm = localtime(&t);
	strftime(sdate, sizeof(sdate), "-1#\"%b %d %Y\"", tm);
	strftime(stime, sizeof(stime), "-1#\"%H:%M:%S\"", tm);
	defmacro("__DATE__")->u.s = sdate;
	defmacro("__TIME__")->u.s = stime;

	defmacro("__STDC_VERSION__")->u.s = "-1#199409L";
	symline = defmacro("__LINE__");
	symfile = defmacro("__FILE__");

	for (bp = list; *bp; ++bp)
		defmacro(*bp)->u.s = "-1#1";
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

static size_t
copymacro(char *buffer, char *s, size_t bufsiz, char *arglist[])
{
	char prevc, c, *arg, *bp = buffer;

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

	return bp - buffer;

expansion_too_long:
	error("expansion of macro \"%s\" is too long", macroname);
}

#define BUFSIZE ((INPUTSIZ > FILENAME_MAX+2) ? INPUTSIZ : FILENAME_MAX+2)
bool
expand(char *begin, Symbol *sym)
{
	size_t total, elen, rlen, llen, ilen;
	int n;
	char *s = sym->u.s;
	char *arglist[NR_MACROARG], arguments[INPUTSIZ], buffer[BUFSIZE];

	macroname = sym->name;
	if (!(sym->flags & ISDECLARED)) {
		/*
		 * This case happens in #if were macro not defined must
		 * be expanded to 0
		 */
		buffer[0] = '0';
		buffer[1] = '\0';
		elen = 1;
		goto substitute;
	}
	if (sym == symfile) {
		elen = sprintf(buffer, "\"%s\" ", input->fname);
		goto substitute;
	}
	if (sym == symline) {
		elen = sprintf(buffer, "%d ", input->nline);
		goto substitute;
	}

	if (!parsepars(arguments, arglist, atoi(s)))
		return 0;
	for (n = 0; n < atoi(s); ++n)
		DBG(stderr, "MACRO par%d:%s\n", n, arglist[n]);

	elen = copymacro(buffer, s+3, INPUTSIZ-1, arglist);

substitute:
	DBG(stderr, "MACRO '%s' expanded to :'%s'\n", macroname, buffer);
	rlen = strlen(input->p);      /* rigth length */
	llen = begin - input->line;   /* left length */
	ilen = input->p - begin;      /* invocation length */
	total = llen + elen + rlen;

	if (total >= LINESIZ)
		error("macro expansion too long");

	/* cut macro invocation */
	memmove(begin, begin + ilen, rlen);

	/* paste macro expansion */
	memmove(begin + elen, begin, rlen);
	memcpy(begin, buffer, elen);
	input->line[total] = '\0';

	input->p = input->begin = begin;

	if (!(sym->flags & ISDECLARED))
		delmacro(sym);

	return 1;
}
#undef BUFSIZE

static int
getpars(Symbol *args[NR_MACROARG])
{
	int n = -1;

	if (!accept('('))
		return n;
	++n;
	if (accept(')'))
		return n;

	do {
		if (n == NR_MACROARG) {
			cpperror("too much parameters in macro");
			return NR_MACROARG;
		}
		if (yytoken != IDEN) {
			cpperror("macro arguments must be identifiers");
			return NR_MACROARG;
		}
		args[n++] = yylval.sym;
		next();
	} while (accept(','));
	expect(')');

	return n;
}

static bool
getdefs(Symbol *args[NR_MACROARG], int nargs, char *bp, size_t bufsiz)
{
	Symbol **argp;
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
				sprintf(yytext, "@%02d@", (int) (argp - args));
				ispar = 1;
			}
		}
		if (prevc == '#' && !ispar) {
			cpperror("'#' is not followed by a macro parameter");
			return 0;
		}
		if (yytoken == EOFTOK)
			break;

		if ((len = strlen(yytext)) >= bufsiz) {
			cpperror("too long macro");
			return 0;
		}
		memcpy(bp, yytext, len);
		bp += len;
		bufsiz -= len;
		if ((prevc = yytoken) != '#')
			*bp++ = ' ';
		next();
	}
	*bp = '\0';
	return 1;
}

static void
define(void)
{
	Symbol *sym,*args[NR_MACROARG];
	char buff[LINESIZ+1];
	int n;

	if (cppoff)
		return;

	setnamespace(NS_CPP);
	next();
	if (yytoken != IDEN) {
		cpperror("macro names must be identifiers");
		return;
	}
	sym = yylval.sym;
	if (sym->flags & ISDECLARED) {
		warn("'%s' redefined", yytext);
		free(sym->u.s);
	} else {
		sym = lookup(NS_CPP);
		sym->flags |= ISDECLARED;
	}

	setnamespace(NS_IDEN);       /* Avoid polution in NS_CPP */
	next();
	if ((n = getpars(args)) == NR_MACROARG)
		goto delete;
	sprintf(buff, "%02d#", n);
	if (!getdefs(args, n, buff+3, LINESIZ-3))
		goto delete;
	sym->u.s = xstrdup(buff);
	DBG(stderr, "MACRO '%s' defined as '%s'\n", sym->name, buff);
	return;

delete:
	delmacro(sym);
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

	setnamespace(NS_IDEN);
	next();

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
		cpperror("included file '%s' not found", file);

	return;

bad_include:
	cpperror("#include expects \"FILENAME\" or <FILENAME>");
	return;
too_long:
	cpperror("#include FILENAME too long");
	return;
}

static void
line(void)
{
	long n;
	char *endp;

	if (cppoff)
		return;

	next();
	n = strtol(yytext, &endp, 10);
	if (n <= 0 || n > USHRT_MAX || *endp != '\0') {
		cpperror("first parameter of #line is not a positive integer");
		return;
	}

	next();
	if (yytoken == EOFTOK)
		goto set_line;

	if (*yytext != '\"' || yylen == 1) {
		cpperror("second parameter of #line is not a valid filename");
		return;
	}

	free(input->fname);
	input->fname = xstrdup(yylval.sym->u.s);
	next();

set_line:
	input->nline = n;
}

static void
pragma(void)
{
	if (cppoff)
		return;
	*input->p = '\0';
	next();
}

static void
usererr(void)
{
	if (cppoff)
		return;
	cpperror("#error %s", input->p);
	*input->p = '\0';
	next();
}

static void
ifclause(int negate, int isifdef)
{
	Symbol *sym;
	unsigned n;
	int status;
	Node *expr;

	if (cppctx == NR_COND-1)
		error("too much nesting levels of conditional inclusion");

	n = cppctx++;
	setnamespace(NS_CPP);
	next();

	if (isifdef) {
		if (yytoken != IDEN) {
			cpperror("no macro name given in #%s directive",
			         (negate) ? "ifndef" : "ifdef");
			return;
		}
		sym = yylval.sym;
		next();
		status = (sym->flags & ISDECLARED) != 0;
		if (!status)
			delmacro(sym);
	} else {
		/* TODO: catch recovery here */
		if ((expr = iconstexpr()) == NULL) {
			cpperror("parameter of #if is not an integer constant expression");
			return;
		}
		status = expr->sym->u.i != 0;
	}

	if (negate)
		status = !status;
	if ((ifstatus[n] = status) == 0)
		++cppoff;
}

static void
cppif(void)
{
	disexpand = 0;
	ifclause(0, 0);
}

static void
ifdef(void)
{
	ifclause(0, 1);
}

static void
ifndef(void)
{
	ifclause(1, 1);
}

static void
elseclause(void)
{
	int status;

	if (cppctx == 0)
		error("#else without #ifdef/ifndef");

	status = (ifstatus[cppctx-1] ^= 1);
	cppoff += (status) ? -1 : 1;
}

static void
cppelse(void)
{
	elseclause();
	next();
}

static void
elif(void)
{
	elseclause();
	cppif();
}

static void
endif(void)
{
	if (cppctx == 0)
		error("#endif without #if");
	if (!ifstatus[--cppctx])
		--cppoff;
	next();
}

static void
undef(void)
{
	if (cppoff)
		return;

	setnamespace(NS_CPP);
	next();
	if (yytoken != IDEN) {
		error("no macro name given in #undef directive");
		return;
	}
	delmacro(yylval.sym);
	next();
}

bool
cpp(void)
{
	static struct {
		uint8_t token;
		void (*fun)(void);
	} *bp, clauses [] = {
		{DEFINE, define},
		{INCLUDE, include},
		{LINE, line},
		{IFDEF, ifdef},
		{IF, cppif},
		{ELIF, elif},
		{IFNDEF, ifndef},
		{ELSE, cppelse},
		{ENDIF, endif},
		{UNDEF, undef},
		{PRAGMA, pragma},
		{ERROR, usererr},
		{0, NULL}
	};

	if (*input->p != '#')
		return 0;
	++input->p;

	disexpand = 1;
	lexmode = CPPMODE;
	setnamespace(NS_CPPCLAUSES);
	next();
	for (bp = clauses; bp->token && bp->token != yytoken; ++bp)
		/* nothing */;
	if (!bp->token)
		error("incorrect preprocessor directive");

	pushctx();              /* create a new context to avoid polish */
	(*bp->fun)();           /* the current context, and to get all  */
	popctx();               /* the symbols freed at the  end        */

	if (yytoken != EOFTOK && !cppoff)
		errorp("trailing characters after preprocessor directive");
	disexpand = 0;
	lexmode = CCMODE;

	return 1;
}
