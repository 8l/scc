
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

/* TODO: preprocessor error must not rise recover */

static char *argp;
static unsigned arglen;
static unsigned numif, iffalse;
static Symbol *lastmacro;

static bool
iden(char **str)
{
	char c, *bp, *s = *str;

	for (bp = yytext; bp < &yytext[IDENTSIZ]; *bp++ = c) {
		if ((c = *s) == '\0' || !isalnum(c) && c != '_')
			break;
		++s;
	}
	if (bp == &yytext[IDENTSIZ])
		error("identifier too long");
	if (bp - yytext == 0)
		return 0;
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
		error("trailing characters after preprocessor directive");
}

static void
nextcpp(void)
{
	next();
	if (yytoken == EOFTOK) {
		error("unterminated argument list invoking macro \"%s\"",
		      lastmacro->name);
	}
	if (yylen + 1 > arglen) {
		error("argument overflow invoking macro \"%s\"",
		      lastmacro->name);
	}
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
			argp -= 2;
			*argp++ = '\0';
			return;
		case '(':
			paren();
			break;
		}
	}
}

static bool
parsepars(char *buffer, char **listp, int nargs)
{
	int n;

	if (nargs == -1)
		return 1;

	if (ahead() != '(')
		return 0;
	next();

	n = 0;
	argp = buffer;
	arglen = INPUTSIZ;
	if (ahead() != ')') {
		do {
			*listp++ = argp;
			parameter();
		} while (++n < NR_MACROARG && yytoken == ',');
	}

	if (n == NR_MACROARG)
		error("too much parameters in macro \"%s\"", lastmacro->name);
	if (n != nargs) {
		error("macro \"%s\" passed %d arguments, but it takes %d",
		      lastmacro->name, n, nargs);
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
bool
expand(Symbol *sym)
{
	unsigned len;
	char *arglist[NR_MACROARG], buffer[INPUTSIZ];
	char c, *bp, *arg, *s = sym->u.s;

	lastmacro = sym;
	if (!parsepars(buffer, arglist, atoi(s)))
		return 0;

	bp = addinput(NULL);
	len = INPUTSIZ-1;
	for (s += 3; c = *s; ++s) {
		if (c != '@') {
			if (len-- == 0)
				goto expansion_too_long;
			*bp++ = c;
		} else {
			unsigned size;

			arg = arglist[atoi(++s)];
			size = strlen(arg);
			if (size > len)
				goto expansion_too_long;
			memcpy(bp, arg, size);
			bp += size;
			len -= size;
			s += 2;
		}
	}
	*bp = '\0';
	return 1;

expansion_too_long:
	error("expansion of macro \"%s\" is too long", lastmacro->name);
}

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
		if (!isalnum(*s) && *s != '_')
			error("macro arguments must be identifiers");
		for (endp = s; isalnum(*endp) || *endp == '_'; ++endp)
			/* nothing */;
		if ((len = endp - s) > IDENTSIZ)
			error("macro argument too long");
		*args++ = s;
		for (s = endp; isspace(*s); ++s)
			*s = '\0';
		c = *s;
		*s++ = '\0';
		if (c == ')')
			break;
		if (c == ',')
			continue;
		else
			error("macro parameters must be comma-separated");
	}
	if (n > NR_MACROARG)
		error("too much parameters in macro");

set_nargs:
	*nargs = n;
	return s;
}
/*
 * Copy a define string, and substitute formal arguments of the
 * macro into strings in the form @XX, where XX is the position
 * of the argument in the argument list.
 */
static char *
copydefine(char *s, char *args[], char *buff, int bufsiz, int nargs)
{
	unsigned ncopy;
	int n;
	size_t len;
	char arroba[6], *par, *endp, **bp;

	while (*s && bufsiz > 0) {
		if (!isalnum(*s) && *s != '_') {
			--bufsiz;
			*buff++ = *s++;
			continue;
		}
		/*
		 * found an identifier, is it one of the macro arguments?
		 */
		for (endp = s+1; isalnum(*endp) || *endp == '_'; ++endp)
			/* nothing */;
		len = endp - s;
		for (bp =args, n = 0; n < nargs; ++bp, n++) {
			if (strncmp(s, *bp, len))
				continue;
			sprintf(arroba, "@%02d@", n);
			break;
		}
		if (n == nargs || nargs == -1)
			par = s, ncopy = len;
		else
			par = arroba, ncopy = 4;

		if ((bufsiz -= ncopy) < 0)
			goto too_long;
		memcpy(buff, par, ncopy);
		buff += ncopy;
		s = endp;
	}

	if (*s == '\0') {
		*buff = '\0';
		return s;
	}

too_long:
	error("macro definition too long");
}

static char *
mkdefine(char *s, Symbol *sym)
{
	int nargs;
	char *args[NR_MACROARG], buff[LINESIZ+1];

	s = parseargs(s, args, &nargs);
	sprintf(buff, "%02d#", nargs);

	while (isspace(*s))
		++s;

	if (*s != '\0')
		s = copydefine(s, args, buff+3, LINESIZ-3, nargs);
	sym->u.s = xstrdup(buff);

	return s;
}

static bool
define(char *s)
{
	char *t;
	Symbol *sym;

	if (!iden(&s))
		error("macro names must be identifiers");

	sym = lookup(NS_CPP);
	if ((sym->flags & ISDEFINED) && sym->ns == NS_CPP) {
		warn("'%s' redefined", yytext);
		free(sym->u.s);
	}
	sym->flags |= ISDEFINED;
	sym->ns = NS_CPP;

	for (t = s + strlen(s) + 1; isspace(*--t); *t = '\0')
		/* nothing */;
	mkdefine(s, sym);
	return 1;
}

static bool
include(char *s)
{
	char delim, c, *p, *file;

	if ((c = *s++) == '>')
		delim = '>';
	else if (c == '"')
		delim = '"';
	else
		goto bad_include;

	if (!string(&s, &file, delim))
		goto bad_include;
	cleanup(s);
	if (delim == '"' && addinput(file))
		return 1;
	abort();

not_found:
	error("included file '%s' not found", s);
bad_include:
	error("#include expects \"FILENAME\" or <FILENAME>");
}

static bool
line(char *s)
{
	char *file;
	long n;

	if ((n = strtol(s, &s, 10)) <= 0 || n > USHRT_MAX)
		error("first parameter of #line is not a positive integer");

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
		cleanup(s);
	case '\0':
	end_string:
		setfline(n-1);
		return 1;
	default:
	bad_file:
		error("second parameter of #line is not a valid filename");
	}
}

static bool
pragma(char *s)
{
	return 1;
}

static bool
usererr(char *s)
{
	error("#error %s", s);
}

static bool
ifclause(char *s, int isdef)
{
	unsigned curif;
	char *endp;
	Symbol *sym;

	if (iden(&s))
		error("...");
	cleanup(s);

	++numif;
	if (iffalse == 0) {
		sym = lookup(NS_CPP);
		if ((sym->flags & ISDEFINED) != 0 == isdef)
			return 1;
	}

	curif = iffalse++;
	while (curif != iffalse) {
		if (!moreinput())
			error("found EOF while ...");
	}

	return 1;
}

static bool
ifdef(char *s)
{
	return ifclause(s, 1);
}

static bool
ifndef(char *s)
{
	return ifclause(s, 0);
}

static bool
endif(char *s)
{
	cleanup(s);
	if (numif == 0)
		error("#endif without #if");
	--numif;
	return iffalse == 0;
}

static bool
elseclause(char *s)
{
	unsigned curif;

	cleanup(s);
	if (numif == 0)
		error("#else without #if");

	if (iffalse == 0) {
		curif = iffalse++;
		while (curif != iffalse) {
			if (!moreinput())
				error("found EOF while ...");
		}
	}
	--iffalse;

	return iffalse != 0;
}

bool
preprocessor(char *s)
{
	static struct {
		char *name;
		bool (*fun)(char *);
	} *bp, cmds[] =  {
		"define", define,
		"include", include,
		"ifdef", ifdef,
		"ifndef", ifndef,
		"endif", endif,
		"else", elseclause,
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
		return (*bp->fun)(s);
	}
incorrect:
	error("invalid preprocessor directive #%s", yytext);
}
