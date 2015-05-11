
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/sizes.h"
#include "../inc/cc.h"
#include "cc1.h"

/* TODO: preprocessor error must not rise recover */
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

	while (isspace(*t))
		++t;
	for (s = t + strlen(t); isspace(*--s); *s = '\0')
		/* nothing */;
	sym->u.s = xstrdup(t);
	return s+1;

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
	static char **bp, *cmds[] = {
		"define",
		"include",
		"line",
		"pragma",
		"error",
		NULL
	};
	static char *(*funs[])(char *) = {
		define,
		include,
		line,
		pragma,
		usererr
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

