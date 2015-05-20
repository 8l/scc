
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

/* TODO: preprocessor error must not rise recover */

/*
 * Parse an argument list (par0, par1, ...) and creates
 * an array with pointers to all the arguments in the
 * list
 */
static char *
parseargs(char *s, char *args[NR_MACROARG], int *nargs)
{
	unsigned n ;
	size_t len;
	char *endp, c;

	if (*s != '(') {
		*nargs = -1;
		return s;
	}
	if (*++s == ')') {
		*nargs = 0;
		return s+1;
	}


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
	unsigned ncopy, n;
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
		if (n == nargs)
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

static char *
define(char *s)
{
	char *t, name[IDENTSIZ+1];
	size_t len;
	Symbol *sym;

	if (!isalnum(*s) && *s != '_')
		goto bad_define;
	for (t = s; isalnum(*t) || *t == '_'; ++t)
		/* nothing */;
	if ((len = t - s) > IDENTSIZ)
		goto too_long;
	strncpy(name, s, len);
	name[len] = '\0';
	sym = install(name, NS_CPP);

	for (s = t; isspace(*s); ++s)
		/* nothing */;
	for (t = s + strlen(s); isspace(*--t); *t = '\0')
		/* nothing */;
	return mkdefine(s, sym);

too_long:
	error("macro identifier too long");
bad_define:
	error("macro names must be identifiers");
}

static char *
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
line(char *s)
{
	char *p, *q;

	if (!isdigit(*p))
		goto bad_line;
	for (p = s; isdigit(*p); ++p)
		/* nothing */;
	switch (*p) {
	case ' ':
	case '\t':
		while (isspace(*p))
			++p;
		if (*p != '"')
			goto bad_line;
		for (q = p+1; *q && *q != '"'; ++q)
			/* nothing */;
		if (*q == '\0')
			goto bad_line;
		*q = '\0';
		setfname(p);
		p = q+1;
		/* passthrough */
	case '\0':
		setfline(atoi(s)-1);
		return p;
	default:
		goto bad_file;
	}

bad_file:
	error("second parameter of #line is not a valid filename");
bad_line:
	error("first parameter of #line is not a positive integer");
}

static char *
pragma(char *s)
{
	while (*s)
		++s;
	return s;
}

static char *
usererr(char *s)
{
	fprintf(stderr, "%s:%u:error: #error %s\n", getfname(), getfline(), s);
	exit(-1);
}

char *
preprocessor(char *p)
{
	char *q;
	unsigned short n;
	static struct {
		char *name;
		char *(*fun)(char *);
	} *bp, cmds[] =  {
		"define", define,
		"include", include,
		"line", line,
		"pragma", pragma,
		"error", usererr,
		NULL, NULL
	};

	while (isspace(*p))
		++p;
	if (*p != '#')
		return p;
	for (++p; isspace(*p); ++p)
		/* nothing */;
	if (*p == '\0')
		return NULL;
	for (q = p; isalpha(*q); ++q)
		/* nothing */;
	if ((n = q - p) == 0)
		goto incorrect;
	while (isspace(*q))
		++q;
	for (bp = cmds; bp->name; ++bp) {
		if (strncmp(bp->name, p, n))
			continue;
		q = (*bp->fun)(q);
		while (isspace(*q++))
			/* nothing */;
		if (*q != '\0')
			error("trailing characters after preprocessor directive");
		return NULL;
	}
incorrect:
	error("incorrect preprocessor directive");
}
