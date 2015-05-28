
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

static char *argp, *macroname;
static unsigned arglen;
static Symbol *symline, *symfile;
static unsigned char ifstatus[NR_COND];
static int paramerr;

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

static bool
iden(char **str)
{
	char c, *bp, *s = *str;

	if (!isalpha(c = *s) && c != '_')
		return 0;
	for (bp = yytext; bp < &yytext[IDENTSIZ]; *bp++ = c) {
		if ((c = *s) == '\0' || !isalnum(c) && c != '_')
			break;
		++s;
	}
	if (bp == &yytext[IDENTSIZ]) {
		printerr("identifier too long in preprocessor");
		return 0;
	}
	*bp = '\0';

	while (isspace(*s))
		++s;

	*str = s;
	return 1;
}

static bool
string(char **input, char **str, char delim)
{
	char c, *s = *input;

	if (str)
		*str = s;

	while ((c = *s) && c != delim)
		++s;
	if (c == '\0')
		return 0;
	*s++ = '\0';
	*input = s;

	return 1;
}

static void
cleanup(char *s)
{
	while (isspace(*s))
		++s;
	if (*s != '\0')
		printerr("trailing characters after preprocessor directive");
}

static void
nextcpp(void)
{
	next();
	if (yytoken == EOFTOK) {
		printerr("unterminated argument list invoking macro \"%s\"",
		      macroname);
		goto mark_error;
	}
	if (yylen + 1 > arglen) {
		printerr("argument overflow invoking macro \"%s\"",
		      macroname);
		goto mark_error;
	}
	memcpy(argp, yytext, yylen);
	argp += yylen;
	*argp++ = ' ';
	arglen -= yylen + 1;
	return;

mark_error:
	paramerr = 1;
	yytoken = 0;
}

static void
paren(void)
{
	while (!paramerr) {
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
	while (!paramerr) {
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
		return 1;

	if (ahead() != '(')
		return 0;

	disexpand = 1;
	next();
	paramerr = n = 0;
	argp = buffer;
	arglen = INPUTSIZ;
	if (ahead() != ')') {
		do {
			*listp++ = argp;
			parameter();
		} while (!paramerr && ++n < NR_MACROARG && yytoken == ',');
	}
	disexpand = 0;

	if (paramerr)
		return -1;
	if (n == NR_MACROARG) {
		printerr("too much parameters in macro \"%s\"", macroname);
		return -1;
	}
	if (n != nargs) {
		printerr("macro \"%s\" passed %d arguments, but it takes %d",
		      macroname, n, nargs);
		return -1;
	}

	return 1;
}

/*
 * sym->u.s is a string with the following format:
 * 	dd#string
 * where dd is the number of arguments of the macro
 * (-1 if it is a macro without arguments), and string
 * is the macro definition, where @dd@ indicates the
 * parameter number dd
 */
#define BUFSIZE ((INPUTSIZ > FILENAME_MAX+2) ? INPUTSIZ : FILENAME_MAX+2)
int
expand(Symbol *sym)
{
	unsigned len;
	int r;
	char *arglist[NR_MACROARG], arguments[INPUTSIZ], buffer[BUFSIZE];
	char prevc, c, *bp, *lim, *arg, *s = sym->u.s;

	if (sym == symfile) {
		sprintf(buffer, "\"%s\"", getfname());
		goto add_macro;
	}
	if (sym == symline) {
		sprintf(buffer, "%d", getfline());
		goto add_macro;
	}

	macroname = sym->name;
	if ((r = parsepars(arguments, arglist, atoi(s))) < 1)
		return r;

	len = INPUTSIZ-1;
	bp = buffer;
	for (prevc = '\0', s += 3; c = *s; prevc = c, ++s) {
		if (c != '@') {
			if (c == '#')
				continue;
			if (len-- == 0)
				goto expansion_too_long;
			*bp++ = c;
		} else {
			unsigned size;

			if (prevc == '#')
				len -= 2;
			arg = arglist[atoi(++s)];
			size = strlen(arg);
			if (size > len)
				goto expansion_too_long;
			if (prevc == '#')
				*bp++ = '"';
			memcpy(bp, arg, size);
			bp += size;
			if (prevc == '#')
				*bp++ = '"';
			len -= size;
			s += 2;
		}
	}
	*bp = '\0';
add_macro:
	addinput(NULL, sym, buffer);
	return 1;

expansion_too_long:
	printerr("expansion of macro \"%s\" is too long", macroname);
	return -1;
}
#undef BUFSIZE

/*
 * Parse an argument list (par0, par1, ...) and creates
 * an array with pointers to all the arguments in the
 * list
 */
static char *
parseargs(char *s, char *args[NR_MACROARG], int *nargs)
{
	int n;
	size_t len;
	char *endp, c;

	n = -1;
	if (*s != '(')
		goto set_nargs;
	n = 0;
	while (isspace(*s++))
		/* nothing */;
	if (*s == ')')
		goto set_nargs;

	for (n = 1; n <= NR_MACROARG; ++n) {
		while (isspace(*s))
			++s;
		if (!isalpha(*s) && *s != '_') {
			printerr("macro arguments must be identifiers");
			return NULL;
		}
		for (endp = s+1; isalnum(*endp) || *endp == '_'; ++endp)
			/* nothing */;
		if ((len = endp - s) > IDENTSIZ) {
			printerr("macro argument too long");
			return NULL;
		}
		*args++ = s;
		for (s = endp; isspace(*s); ++s)
			*s = '\0';
		c = *s;
		*s++ = '\0';
		if (c == ')')
			break;
		if (c == ',') {
			continue;
		} else {
			printerr("macro parameters must be comma-separated");
			return NULL;
		}
	}
	if (n > NR_MACROARG) {
		printerr("too much parameters in macro");
		return NULL;
	}

set_nargs:
	*nargs = n;
	return s;
}

/*
 * Copy a string define, and substitute formal arguments of the
 * macro into strings in the form @XX@, where XX is the position
 * of the argument in the argument list.
 */
static bool
copydefine(char *s, char *args[], char *buff, int bufsiz, int nargs)
{
	int n;
	size_t ncopy;
	char arroba[6], *p, **bp, c, prevc;

	for (prevc = '\0'; c = *s++; prevc = c) {
		if (!isalpha(c) && c != '_' || nargs < 1) {
			if (bufsiz-- == 0)
				goto too_long;
			if (prevc == '#')
				goto bad_stringer;
			*buff++ = c;
			if (c != '#')
				continue;
			while (isspace(*++s))
				/* nothing */;
		}
		/* found an identifier, is it one of the macro arguments? */
		for (p = s; isalnum(c = *p) || c == '_'; ++p)
			/* nothing */;
		ncopy = p - --s;
		bp = args;
		for (n = 0; n < nargs; ++n) {
			if (strncmp(s, *bp++, ncopy))
				continue;
			sprintf(arroba, "@%02d@", n);
			s = arroba, ncopy = 4;
			break;
		}
		if (n == nargs && prevc == '#')
			goto bad_stringer;
		if ((bufsiz -= ncopy) < 0)
			goto too_long;
		memcpy(buff, s, ncopy);
		buff += ncopy, s = p;
	}
	if (bufsiz == 0)
		goto too_long;
	*buff = '\0';
	return 1;

bad_stringer:
	printerr("'#' is not followed by a macro parameter");
	return 0;
too_long:
	printerr("macro definition too long");
	return 0;
}

static char *
mkdefine(char *s)
{
	int nargs;
	char *args[NR_MACROARG], buff[LINESIZ+1];

	if ((s = parseargs(s, args, &nargs)) == NULL)
		return NULL;
	sprintf(buff, "%02d#", nargs);

	while (isspace(*s))
		++s;

	if (*s == '\0')
		buff[0] = '\0';
	else if (!copydefine(s, args, buff+3, LINESIZ-3, nargs))
		return NULL;
	return xstrdup(buff);
}

static void
define(char *s)
{
	char *t;
	Symbol *sym;

	if (cppoff)
		return;
	if (!iden(&s)) {
		printerr("#define must have an identifier as parameter");
		return;
	}

	for (t = s + strlen(s) + 1; isspace(*--t); *t = '\0')
		/* nothing */;
	if ((s = mkdefine(s)) == NULL)
		return;

	sym = lookup(NS_CPP);
	if ((sym->flags & ISDEFINED) && sym->ns == NS_CPP) {
		warn("'%s' redefined", yytext);
		free(sym->u.s);
	}
	sym->flags |= ISDEFINED;
	sym->ns = NS_CPP;
	sym->ctx = UCHAR_MAX;
	sym->u.s = s;
}

static void
include(char *s)
{
	char **bp, delim, c, *p, *file, path[FILENAME_MAX];
	char *sysinclude[] = {
		PREFIX"/include/",
		PREFIX"/local/include/",
		NULL
	};
	size_t filelen, dirlen;

	if (cppoff)
		return;
	if ((c = *s++) == '>')
		delim = '>';
	else if (c == '"')
		delim = '"';
	else
		goto bad_include;

	if (!string(&s, &file, delim))
		goto bad_include;
	if (delim == '"' && addinput(file, NULL, NULL))
		return;

	filelen = strlen(file);
	for (bp = sysinclude; *bp; ++bp) {
		dirlen = strlen(*bp);
		if (dirlen + filelen > FILENAME_MAX)
			continue;
		memcpy(path, *bp, dirlen);
		memcpy(path+dirlen, file, filelen);
		if (addinput(path, NULL, NULL))
			break;
	}
	if (*bp)
		printerr("included file '%s' not found", file);
	cleanup(s);
	return;

bad_include:
	printerr("#include expects \"FILENAME\" or <FILENAME>");
}

static void
line(char *s)
{
	char *file;
	long n;

	if (cppoff)
		return;
	if ((n = strtol(s, &s, 10)) <= 0 || n > USHRT_MAX) {
		printerr("first parameter of #line is not a positive integer");
		return;
	}

	switch (*s) {
	case ' ':
	case '\t':
		while (isspace(*s))
			++s;
		if (*s == '\0')
			goto end_string;
		if (*s++ != '"' && !string(&s, &file, '"'))
			goto bad_file;
		setfname(file);
	case '\0':
	end_string:
		setfline(n-1);
		break;;
	default:
	bad_file:
		printerr("second parameter of #line is not a valid filename");
		break;
	}
	cleanup(s);
}

static void
pragma(char *s)
{
	if (cppoff)
		return;
}

static void
usererr(char *s)
{
	if (cppoff)
		return;
	printerr("#error %s", s);
	exit(-1);
}

static void
ifclause(char *s, int isdef)
{
	Symbol *sym;
	unsigned n = cppctx++;

	if (cppctx == NR_COND-1) {
		printerr("too much nesting levels of conditional inclusion");
		return;
	}
	if (!iden(&s)) {
		printerr("no macro name given in #%s directive",
		      (isdef) ? "ifdef" : "ifndef");
		return;
	}
	sym = lookup(NS_CPP);
	if (!(ifstatus[n] = (sym->flags & ISDEFINED) != 0 == isdef))
		++cppoff;
	cleanup(s);
}

static void
ifdef(char *s)
{
	ifclause(s, 1);
}

static void
ifndef(char *s)
{
	ifclause(s, 0);
}

static void
endif(char *s)
{
	if (cppctx == 0) {
		printerr("#endif without #if");
		return;
	}
	if (!ifstatus[--cppctx])
		--cppoff;
	cleanup(s);
}

static void
elseclause(char *s)
{
	struct ifstatus *ip;

	if (cppctx == 0) {
		printerr("#else without #ifdef/ifndef");
		return;
	}
	cppoff += (ifstatus[cppctx-1] ^= 1) ? -1 : 1;
	cleanup(s);
}

static void
undef(char *s)
{
	Symbol *sym;

	if (!iden(&s)) {
		printerr("no macro name given in #undef directive");
		return;
	}
	sym = lookup(NS_CPP);
	sym->flags &= ~ISDEFINED;
	cleanup(s);
}

bool
cpp(char *s)
{
	static struct {
		char *name;
		void (*fun)(char *);
	} *bp, cmds[] =  {
		"define", define,
		"include", include,
		"ifdef", ifdef,
		"ifndef", ifndef,
		"endif", endif,
		"else", elseclause,
		"undef", undef,
		"line", line,
		"pragma", pragma,
		"error", usererr,
		NULL, NULL
	};

	if (*s++ != '#')
		return 0;
	while (isspace(*s))
		++s;
	if (!iden(&s))
		goto incorrect;
	for (bp = cmds; bp->name; ++bp) {
		if (strcmp(bp->name, yytext))
			continue;
		(*bp->fun)(s);
		return 1;
	}
incorrect:
	printerr("invalid preprocessor directive #%s", yytext);
	return 1;
}
