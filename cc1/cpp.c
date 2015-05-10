
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

/* TODO: preprocessor error must not rise recover */
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
		goto bad_line;
	}

bad_line:
	error("incorrect #line directive");
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
		"include",
		"line",
		"pragma",
		"error",
		NULL
	};
	static char *(*funs[])(char *) = {
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

