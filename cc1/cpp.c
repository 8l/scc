/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc1/cpp.c";
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cstd.h>
#include "../inc/cc.h"
#include "cc1.h"

extern char *sysincludes[];
static char *argp, *macroname;
static unsigned arglen;
static unsigned ncmdlines;
static Symbol *symline, *symfile;
static unsigned char ifstatus[NR_COND];
static int cppoff;
static struct items dirinclude;

unsigned cppctx;
int disexpand;

void
defdefine(char *macro, char *val, char *source)
{
	char *def, *fmt = "#define %s %s\n";
	Symbol dummy = {.flags = SDECLARED};

	if (!val)
		val = "";
	def = xmalloc(strlen(fmt) + strlen(macro) + strlen(val));

	sprintf(def, fmt, macro, val);
	lineno = ++ncmdlines;
	addinput(source, &dummy, def);
	cpp();
	delinput();
}

void
undefmacro(char *s)
{
	killsym(lookup(NS_CPP, s, NOALLOC));
}

void
icpp(void)
{
	static char sdate[14], stime[11];
	struct tm *tm;
	time_t t;
	static char **bp, *list[] = {
		"__STDC__",
		"__STDC_HOSTED__",
		"__SCC__",
		NULL
	};
	static struct keyword keys[] = {
		{"define", DEFINE, DEFINE},
		{"include", INCLUDE, INCLUDE},
		{"line", LINE, LINE},
		{"ifdef", IFDEF, IFDEF},
		{"if", IF, IF},
		{"elif", ELIF, ELIF},
		{"else", ELSE, ELSE},
		{"ifndef", IFNDEF, IFNDEF},
		{"endif", ENDIF, ENDIF},
		{"undef", UNDEF, UNDEF},
		{"pragma", PRAGMA, PRAGMA},
		{"error", ERROR, ERROR},
		{NULL, 0, 0}
	};

	keywords(keys, NS_CPPCLAUSES);

	t = time(NULL);
	tm = localtime(&t);
	strftime(sdate, sizeof(sdate), "\"%b %d %Y\"", tm);
	strftime(stime, sizeof(stime), "\"%H:%M:%S\"", tm);
	defdefine("__DATE__", sdate, "built-in");
	defdefine("__TIME__", stime, "built-in");
	defdefine("__STDC_VERSION__", STDC_VERSION, "built-in");
	defdefine("__LINE__", NULL, "built-in");
	defdefine("__FILE__", NULL, "built-in");

	symline = lookup(NS_CPP, "__LINE__", ALLOC);
	symfile = lookup(NS_CPP, "__FILE__", ALLOC);

	for (bp = list; *bp; ++bp)
		defdefine(*bp, "1", "built-in");

	ncmdlines = 0;
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
	if (yytoken == IDEN)
		yylval.sym->flags |= SUSED;
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
	if (ahead() == ')') {
		next();
	} else {
		do {
			*listp++ = argp;
			parameter();
		} while (++n < NR_MACROARG && yytoken == ',');
	}
	if (yytoken != ')')
		error("incorrect macro function-alike invocation");
	disexpand = 0;

	if (n == NR_MACROARG)
		error("too many parameters in macro \"%s\"", macroname);
	if (n != nargs) {
		error("macro \"%s\" received %d arguments, but it takes %d",
		      macroname, n, nargs);
	}

	return 1;
}

static size_t
copymacro(char *buffer, char *s, size_t bufsiz, char *arglist[])
{
	char delim, prevc, c, *p, *arg, *bp = buffer;
	size_t size;

	for (prevc = '\0'; c = *s; prevc = c, ++s) {
		switch (c) {
		case '$':
			while (bp[-1] == ' ')
				--bp, ++bufsiz;
			while (s[1] == ' ')
				++s;
		case '#':
			break;
		case '\'':
			delim = '\'';
			goto search_delim;
		case '\"':
			delim = '"';
		search_delim:
			for (p = s; *++s != delim; )
				/* nothing */;
			size = s - p + 1;
			if (size > bufsiz)
				goto expansion_too_long;
			memcpy(bp, p, size);
			bufsiz -= size;
			bp += size;
			break;
		case '@':
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
			break;
		default:
			if (bufsiz-- == 0)
				goto expansion_too_long;
			*bp++ = c;
			break;
		}
	}
	*bp = '\0';

	return bp - buffer;

expansion_too_long:
	error("macro expansion of \"%s\" too long", macroname);
}

int
expand(char *begin, Symbol *sym)
{
	size_t elen;
	int n, i;
	char *s = sym->u.s;
	char *arglist[NR_MACROARG], arguments[INPUTSIZ], buffer[INPUTSIZ];

	macroname = sym->name;
	if (sym == symfile) {
		elen = sprintf(buffer, "\"%s\" ", filenam);
		goto substitute;
	}
	if (sym == symline) {
		elen = sprintf(buffer, "%d ", lineno);
		goto substitute;
	}
	if (!s)
		return 1;

	n = atoi(s);
	if (!parsepars(arguments, arglist, atoi(s)))
		return 0;
	for (i = 0; i < n; ++i)
		DBG("MACRO par%d:%s", i, arglist[i]);

	elen = copymacro(buffer, s+3, INPUTSIZ-1, arglist);

substitute:
	DBG("MACRO '%s' expanded to :'%s'", macroname, buffer);
	buffer[elen] = '\0';
	addinput(filenam, sym, xstrdup(buffer));

	return 1;
}

static int
getpars(Symbol *args[NR_MACROARG])
{
	int n, c;
	Symbol *sym;

	c = *input->p;
	next();
	if (c != '(')
		return -1;
	next(); /* skip the '(' */
	if (accept(')'))
		return 0;

	n = 0;
	do {
		if (n == NR_MACROARG) {
			cpperror("too many parameters in macro");
			return NR_MACROARG;
		}
		if (accept(ELLIPSIS)) {
			args[n++] = NULL;
			break;
		}
		if (yytoken != IDEN) {
			cpperror("macro arguments must be identifiers");
			return NR_MACROARG;
		}
		sym = install(NS_IDEN, yylval.sym);
		sym->flags |= SUSED;
		args[n++] = sym;
		next();
	} while (accept(','));
	expect(')');

	return n;
}

static int
getdefs(Symbol *args[NR_MACROARG], int nargs, char *bp, size_t bufsiz)
{
	Symbol **argp;
	size_t len;
	int prevc = 0, ispar;

	if (yytoken == '$') {
		cpperror("'##' cannot appear at either ends of a macro expansion");
		return 0;
	}

	for (;;) {
		ispar = 0;
		if (yytoken == IDEN && nargs >= 0) {
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
		if (yytoken == '\n')
			break;

		if ((len = strlen(yytext)) >= bufsiz) {
			cpperror("macro too long");
			return 0;
		}
		if (yytoken == '$') {
			*bp++ = '$';
			 --bufsiz;
		} else {
			memcpy(bp, yytext, len);
			bp += len;
			bufsiz -= len;
		}
		if ((prevc = yytoken) != '#') {
			*bp++ = ' ';
			--bufsiz;
		}
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

	namespace = NS_CPP;
	next();

	if (yytoken != IDEN) {
		cpperror("macro names must be identifiers");
		return;
	}
	sym = yylval.sym;
	if (sym->flags & SDECLARED) {
		warn("'%s' redefined", yytext);
		free(sym->u.s);
	} else {
		sym = install(NS_CPP, sym);
		sym->flags |= SDECLARED|SSTRING;
	}

	namespace = NS_IDEN;       /* Avoid polution in NS_CPP */
	if ((n = getpars(args)) == NR_MACROARG)
		goto delete;
	if (n > 0 && !args[n-1])  /* it is a variadic function */
		--n;
	sprintf(buff, "%02d#", n);
	if (!getdefs(args, n, buff+3, LINESIZ-3))
		goto delete;
	sym->u.s = xstrdup(buff);
	DBG("MACRO '%s' defined as '%s'", sym->name, buff);
	return;

delete:
	killsym(sym);
}

void
incdir(char *dir)
{
	if (!dir || *dir == '\0')
		die("incorrect -I flag");
	newitem(&dirinclude, dir);
}

static int
includefile(char *dir, char *file, size_t filelen)
{
	size_t dirlen;
	char path[FILENAME_MAX];

	if (!dir) {
		dirlen = 0;
		if (filelen > FILENAME_MAX-1)
			return 0;
	} else {
		dirlen = strlen(dir);
		if (dirlen + filelen > FILENAME_MAX-2)
			return 0;
		memcpy(path, dir, dirlen);
		if (dir[dirlen-1] != '/')
			path[dirlen++] = '/';
	}
	memcpy(path+dirlen, file, filelen);
	path[dirlen + filelen] = '\0';

	return addinput(path, NULL, NULL);
}

static char *
cwd(char *buf)
{
	char *p, *s = filenam;
	size_t len;

	if ((p = strrchr(s, '/')) == NULL)
		return NULL;
	if ((len = p - s) >= FILENAME_MAX)
		die("current work directory too long");
	memcpy(buf, s, len);
	buf[len] = '\0';
	return buf;
}

static void
include(void)
{
	char dir[FILENAME_MAX], file[FILENAME_MAX], *p, **bp;
	size_t filelen;
	int n;

	if (cppoff)
		return;

	namespace = NS_IDEN;
	next();

	switch (*yytext) {
	case '<':
		if ((p = strchr(input->begin, '>')) == NULL || p[-1] == '<')
			goto bad_include;
		filelen = p - input->begin;
		if (filelen >= FILENAME_MAX)
			goto too_long;
		memcpy(file, input->begin, filelen);
		file[filelen] = '\0';

		input->begin = input->p = p+1;
		if (next() != '\n')
			goto trailing_characters;

		break;
	case '"':
		if (yylen < 3)
			goto bad_include;
		filelen = yylen-2;
		if (filelen >= FILENAME_MAX)
			goto too_long;
		memcpy(file, yytext+1, filelen);
		file[filelen] = '\0';

		if (next() != '\n')
			goto trailing_characters;

		if (includefile(cwd(dir), file, filelen))
			goto its_done;
		break;
	default:
		goto bad_include;
	}

	n = dirinclude.n;
	for (bp = dirinclude.s; n--; ++bp) {
		if (includefile(*bp, file, filelen))
			goto its_done;
	}
	for (bp = sysincludes; *bp; ++bp) {
		if (includefile(*bp, file, filelen))
			goto its_done;
	}
	cpperror("included file '%s' not found", file);

its_done:
	return;

trailing_characters:
	cpperror("trailing characters after preprocessor directive");
	return;

too_long:
	cpperror("too long file name in #include");
	return;

bad_include:
	cpperror("#include expects \"FILENAME\" or <FILENAME>");
	return;
}

static void
line(void)
{
	long n;
	char *endp, *fname;

	if (cppoff)
		return;

	disexpand = 0;
	next();
	n = strtol(yytext, &endp, 10);
	if (n <= 0 || n > USHRT_MAX || *endp != '\0') {
		cpperror("first parameter of #line is not a positive integer");
		return;
	}

	next();
	if (yytoken == '\n') {
		fname = NULL;
	} else {
		if (*yytext != '\"' || yylen == 1) {
			cpperror("second parameter of #line is not a valid filename");
			return;
		}
		fname = yylval.sym->u.s;
	}
	setloc(fname, n - 1);
	if (yytoken != '\n')
		next();
}

static void
pragma(void)
{
	static char magic[] = {
	#include "stallman.msg"
	};

	if (cppoff)
		return;
	next();
	if (!strcmp(yytext, "GCC"))
		warn(magic);
	warn("ignoring pragma '%s'", yytext);
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
		error("too many nesting levels of conditional inclusion");

	n = cppctx++;
	namespace = NS_CPP;
	next();

	if (isifdef) {
		if (yytoken != IDEN) {
			cpperror("no macro name given in #%s directive",
			         (negate) ? "ifndef" : "ifdef");
			return;
		}
		sym = yylval.sym;
		next();
		status = (sym->flags & SDECLARED) != 0;
		if (!status)
			killsym(sym);
	} else {
		/* TODO: catch recovery here */
		if ((expr = constexpr()) == NULL) {
			cpperror("parameter of #if is not an integer constant expression");
			return;
		}
		status = expr->sym->u.i != 0;
		freetree(expr);
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

	if (cppctx == 0) {
		cpperror("#else without #ifdef/ifndef");
		return;
	}

	status = ifstatus[cppctx-1];
	ifstatus[cppctx-1] = !status;
	cppoff += (status) ? 1 : -1;
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
	if (ifstatus[cppctx-1]) {
		--cppctx;
		cppif();
	}
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

	namespace = NS_CPP;
	next();
	if (yytoken != IDEN) {
		error("no macro name given in #undef directive");
		return;
	}
	killsym(yylval.sym);
	next();
}

int
cpp(void)
{
	static struct {
		unsigned char token;
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
	int ns;
	char *p;

	for (p = input->p; isspace(*p); ++p)
		/* nothing */;

	if (*p != '#')
		return cppoff;
	input->p = p+1;

	disexpand = 1;
	lexmode = CPPMODE;
	ns = namespace;
	namespace = NS_CPPCLAUSES;
	next();
	namespace = NS_IDEN;

	for (bp = clauses; bp->token && bp->token != yytoken; ++bp)
		/* nothing */;
	if (!bp->token) {
		errorp("incorrect preprocessor directive '%s'", yytext);
		goto error;
	}

	DBG("CPP %s", yytext);

	pushctx();              /* create a new context to avoid polish */
	(*bp->fun)();           /* the current context, and to get all  */
	popctx();               /* the symbols freed at the  end        */

	/*
	 * #include changes the content of input->line, so the correctness
	 * of the line must be checked in the own include(), and we have
	 * to skip this tests. For the same reason include() is the only
	 * function which does not prepare the next token
	 */
	if (yytoken != '\n' && !cppoff && bp->token != INCLUDE)
		errorp("trailing characters after preprocessor directive");

error:
	disexpand = 0;
	lexmode = CCMODE;
	namespace = ns;

	return 1;
}

void
ppragmaln(void)
{
	static char file[FILENAME_MAX];
	static unsigned nline;
	char *s;

	putchar('\n');
	if (strcmp(file, filenam)) {
		strcpy(file, filenam);
		s = "#line %u \"%s\"\n";
	} else if (nline+1 != lineno) {
		s = "#line %u\n";
	} else {
		s = "";
	}
	nline = lineno;
	printf(s, nline, file);
}

void
outcpp(void)
{
	char c, *s, *t;

	for (next(); yytoken != EOFTOK; next()) {
		if (onlyheader)
			continue;
		if (yytoken != STRING) {
			printf("%s ", yytext);
			continue;
		}
		for (s = yytext; c = *s; ++s) {
			switch (c) {
			case '\n':
				t = "\\n";
				goto print_str;
			case '\v':
				t = "\\v";
				goto print_str;
			case '\b':
				t = "\\b";
				goto print_str;
			case '\t':
				t = "\\t";
				goto print_str;
			case '\a':
				t = "\\a";
			print_str:
				fputs(t, stdout);
				break;
			case '\\':
				putchar('\\');
			default:
				if (!isprint(c))
					printf("\\x%x", c);
				else
					putchar(c);
				break;
			}
		}
		putchar(' ');
	}
	putchar('\n');
}

