
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

/* TODO: preprocessor error must not rise recover */
char *
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
pragma(char *s)
{
	while (*s)
		++s;
	return s;
}

char *
preprocessor(char *p)
{
	char *q;
	unsigned short n;
	static char **bp, *cmds[] = {
		"include",
		"pragma",
		NULL
	};
	static char *(*funs[])(char *) = {
		include,
		pragma
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

