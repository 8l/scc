/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./driver/posix/scc.c";
#define _POSIX_SOURCE
#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/arg.h"
#include "../../inc/cc.h"

enum {
	CC1,
	TEEIR,
	CC2,
	TEEQBE,
	QBE,
	TEEAS,
	AS,
	LD,
	STRIP,
	LAST_TOOL,
};

static struct tool {
	char   cmd[PATH_MAX];
	char   bin[16];
	char  *outfile;
	struct items args;
	unsigned nparams;
	int    in, out, init;
	pid_t  pid;
} tools[] = {
	[CC1]    = { .cmd = "cc1" },
	[TEEIR]  = { .bin = "tee",   .cmd = "tee", },
	[CC2]    = { .cmd = "cc2" },
	[TEEQBE] = { .bin = "tee",   .cmd = "tee", },
	[QBE]    = { .bin = "qbe",   .cmd = "qbe", },
	[TEEAS]  = { .bin = "tee",   .cmd = "tee", },
	[AS]     = { .bin = "as",    .cmd = "as", },
	[LD]     = { .bin = "gcc",   .cmd = "gcc", }, /* TODO use ld */
	[STRIP]  = { .bin = "strip", .cmd = "strip", },
};

char *argv0;
static char *arch, *execpath, *objfile, *outfile;
static char *tmpdir;
static size_t tmpdirln;
static struct items objtmp, objout;
static int Mflag, Eflag, Sflag, cflag, dflag, kflag, sflag;
static int devnullfd;

extern int failure;

static void
terminate(void)
{
	unsigned i;

	if (!kflag) {
		for (i = 0; i < objtmp.n; ++i)
			unlink(objtmp.s[i]);
	}
}

static void
addarg(int tool, char *arg)
{
	struct tool *t = &tools[tool];

	if (t->args.n < 1)
		t->args.n = 1;

	newitem(&t->args, arg);
}

static void
setargv0(int tool, char *arg)
{
	struct tool *t = &tools[tool];

	if (t->args.n > 0)
		t->args.s[0] = arg;
	else
		newitem(&t->args, arg);
}

static int
inittool(int tool)
{
	struct tool *t = &tools[tool];
	int n;

	if (t->init)
		return tool;

	switch (tool) {
	case CC1: /* FALLTHROUGH */
	case CC2:
		n = snprintf(t->bin, sizeof(t->bin), "%s-%s", t->cmd, arch);
		if (n < 0 || n >= sizeof(t->bin))
			die("scc: target tool name too long");

		n = snprintf(t->cmd, sizeof(t->cmd), "%s/%s", execpath, t->bin);
		if (n < 0 || n >= sizeof(t->cmd))
			die("scc: target tool path too long");
		break;
	case LD:
		addarg(tool, "-no-pie");
		addarg(tool, "-o");
		t->outfile = outfile ? outfile : xstrdup("a.out");
		addarg(tool, t->outfile);
		break;
	case AS:
		addarg(tool, "-o");
		break;
	default:
		break;
	}

	setargv0(tool, t->bin);
	t->nparams = t->args.n;
	t->init = 1;

	return tool;
}

static char *
outfname(char *path, char *type)
{
	char *new, sep, *p;
	size_t newsz, pathln;
	int tmpfd, n;

	if (path) {
		sep = '.';
		if (p = strrchr(path, '/'))
			path = p + 1;
		pathln = strlen(path);
		if (p = strrchr(path, '.'))
			pathln -= strlen(p);
	} else {
		sep = '/';
		type = "scc-XXXXXX";
		path = tmpdir;
		pathln = tmpdirln;
	}

	newsz = pathln + 1 + strlen(type) + 1;
	new = xmalloc(newsz);
	n = snprintf(new, newsz, "%.*s%c%s", (int)pathln, path, sep, type);
	if (n < 0 || n >= newsz)
		die("scc: wrong output filename");
	if (sep == '/') {
		if ((tmpfd = mkstemp(new)) < 0)
			die("scc: could not create output file '%s': %s",
			    new, strerror(errno));
		close(tmpfd);
	}

	return new;
}

static int
settool(int tool, char *infile, int nexttool)
{
	struct tool *t = &tools[tool];
	unsigned i;
	int fds[2];
	static int fdin = -1;

	switch (tool) {
	case TEEIR:
		t->outfile = outfname(infile, "ir");
		addarg(tool, t->outfile);
		break;
	case TEEQBE:
		t->outfile = outfname(infile, "qbe");
		addarg(tool, t->outfile);
		break;
	case TEEAS:
		t->outfile = outfname(infile, "s");
		addarg(tool, t->outfile);
		break;
	case AS:
		if (cflag && outfile) {
			objfile = outfile;
		} else {
			objfile = (cflag || kflag) ? infile : NULL;
			objfile = outfname(objfile, "o");
		}
		t->outfile = xstrdup(objfile);
		addarg(tool, t->outfile);
		break;
	case STRIP:
		if (cflag || kflag) {
			for (i = 0; i < objout.n; ++i)
				addarg(tool, xstrdup(objout.s[i]));
		}
		if (!cflag && tools[LD].outfile)
			addarg(tool, tools[LD].outfile);
		break;
	default:
		break;
	}

	if (fdin > -1) {
		t->in = fdin;
		fdin = -1;
	} else {
		t->in = -1;
		if (infile)
			addarg(tool, xstrdup(infile));
	}

	if (nexttool < LAST_TOOL) {
		if (pipe(fds))
			die("scc: pipe: %s", strerror(errno));
		t->out = fds[1];
		fdin = fds[0];
	} else {
		t->out = -1;
	}

	addarg(tool, NULL);

	return tool;
}

static void
spawn(int tool)
{
	struct tool *t = &tools[tool];

	switch (t->pid = fork()) {
	case -1:
		die("scc: %s: %s", t->bin, strerror(errno));
	case 0:
		if (t->out > -1)
			dup2(t->out, 1);
		if (t->in > -1)
			dup2(t->in, 0);
		if (!dflag && tool != CC1)
			dup2(devnullfd, 2);
		execvp(t->cmd, t->args.s);
		fprintf(stderr, "scc: execvp %s: %s\n",
		        t->cmd, strerror(errno));
		_exit(1);
	default:
		if (t->in > -1)
			close(t->in);
		if (t->out > -1)
			close(t->out);
		break;
	}
}

static int
toolfor(char *file)
{
	char *dot = strrchr(file, '.');

	if (dot) {
		if (!strcmp(dot, ".c"))
			return CC1;
		if (!strcmp(dot, ".ir"))
			return CC2;
		if (!strcmp(dot, ".qbe"))
			return QBE;
		if (!strcmp(dot, ".s"))
			return AS;
		if (!strcmp(dot, ".o"))
			return LD;
		if (!strcmp(dot, ".a"))
			return LD;
	} else if (!strcmp(file, "-")) {
		return CC1;
	}

	die("scc: do not recognize filetype of %s", file);
}

static int
validatetools(void)
{
	struct tool *t;
	unsigned i;
	int tool, st, failed = LAST_TOOL;

	for (tool = 0; tool < LAST_TOOL; ++tool) {
		t = &tools[tool];
		if (!t->pid)
			continue;
		if (waitpid(t->pid, &st, 0) < 0 ||
		    !WIFEXITED(st) ||
		    WEXITSTATUS(st) != 0) {
			if (!WIFEXITED(st)) {
				fprintf(stderr,
				        "scc:%s: internal error\n", t->bin);
			}
			failure = 1;
			failed = tool;
		}
		if (tool >= failed && t->outfile)
			unlink(t->outfile);
		for (i = t->nparams; i < t->args.n; ++i)
			free(t->args.s[i]);
		t->args.n = t->nparams;
		t->pid = 0;
	}
	if (failed < LAST_TOOL) {
		unlink(objfile);
		free(objfile);
		objfile = NULL;
		return 0;
	}

	return 1;
}

static int
buildfile(char *file, int tool)
{
	int nexttool;

	for (; tool < LAST_TOOL; tool = nexttool) {
		switch (tool) {
		case CC1:
			if (Eflag || Mflag)
				nexttool = LAST_TOOL;
			else
				nexttool = kflag ? TEEIR : CC2;
			break;
		case TEEIR:
			nexttool = CC2;
			break;
		case CC2:
			if (!arch || strcmp(arch, "qbe"))
				nexttool = (Sflag || kflag) ? TEEAS : AS;
			else
				nexttool = kflag ? TEEQBE : QBE;
			break;
		case TEEQBE:
			nexttool = QBE;
			break;
		case QBE:
			nexttool = (Sflag || kflag) ? TEEAS : AS;
			break;
		case TEEAS:
			nexttool = Sflag ? LAST_TOOL : AS;
			break;
		case AS:
			nexttool = LAST_TOOL;
			break;
		default:
			nexttool = LAST_TOOL;
			continue;
		}

		spawn(settool(inittool(tool), file, nexttool));
	}

	return validatetools();
}

static void
build(struct items *chain, int link)
{
	int i, tool;

	if (link)
		inittool(LD);

	for (i = 0; i < chain->n; ++i) {
		if (!strcmp(chain->s[i], "-l")) {
			if (link) {
				addarg(LD, xstrdup(chain->s[i++]));
				addarg(LD, xstrdup(chain->s[i]));
			} else {
				++i;
			}
			continue;
		}
		tool = toolfor(chain->s[i]);
		if (tool == LD) {
			if (link)
				addarg(LD, xstrdup(chain->s[i]));
			continue;
		}
		if (buildfile(chain->s[i], tool)) {
			if (link)
				addarg(LD, xstrdup(objfile));
			newitem((!link || kflag) ? &objout : &objtmp, objfile);
		}
	}
}

static void
usage(void)
{
	die("usage: scc [-D def[=val]]... [-U def]... [-I dir]... "
	    "[-L dir]... [-l dir]...\n"
	    "           [-dgksw] [-m arch] [-M|-E|-S] [-o outfile] file...\n"
	    "       scc [-D def[=val]]... [-U def]... [-I dir]... "
	    "[-L dir]... [-l dir]...\n"
	    "           [-dgksw] [-m arch] [-M|-E|-S] -c file...\n"
	    "       scc [-D def[=val]]... [-U def]... [-I dir]... "
	    "[-L dir]... [-l dir]...\n"
	    "           [-dgksw] [-m arch] -c -o outfile file");
}

int
main(int argc, char *argv[])
{
	struct items linkchain = { .n = 0, };
	int link;

	atexit(terminate);

	if (!(arch = getenv("ARCH")))
		arch = ARCH;
	if (!(execpath = getenv("SCCEXECPATH")))
		execpath = PREFIX "/libexec/scc";

	ARGBEGIN {
	case 'D':
		addarg(CC1, "-D");
		addarg(CC1, EARGF(usage()));
		break;
	case 'M':
		Mflag = 1;
		addarg(CC1, "-M");
		break;
	case 'E':
		Eflag = 1;
		addarg(CC1, "-E");
		break;
	case 'I':
		addarg(CC1, "-I");
		addarg(CC1, EARGF(usage()));
		break;
	case 'L':
		addarg(LD, "-L");
		addarg(LD, EARGF(usage()));
		break;
	case 'O':
		EARGF(usage());
		break;
	case 'S':
		Sflag = 1;
		break;
	case 'U':
		addarg(CC1, "-U");
		addarg(CC1, EARGF(usage()));
		break;
	case 'c':
		cflag = 1;
		break;
	case 'd':
		dflag = 1;
		break;
	case 'g':
		addarg(AS, "-g");
		addarg(LD, "-g");
		break;
	case 'k':
		kflag = 1;
		break;
	case 'l':
		newitem(&linkchain, "-l");
		newitem(&linkchain, EARGF(usage()));
		break;
	case 'm':
		arch = EARGF(usage());
		break;
	case 'o':
		outfile = xstrdup(EARGF(usage()));
		break;
	case 's':
		sflag = 1;
		break;
	case 'W':
		EARGF(usage());
	case 'w':
		addarg(CC1, "-w");
		break;
	case '-':
		fprintf(stderr,
		        "scc: ignored parameter --%s\n", EARGF(usage()));
		break;
	default:
		usage();
	} ARGOPERAND {
operand:
		newitem(&linkchain, ARGOP());
	} ARGEND

	for (; *argv; --argc, ++argv)
		goto operand;

	if (Eflag && linkchain.n == 0)
		newitem(&linkchain, "-");

	if (Eflag && Mflag ||
            (Eflag || Mflag) && (Sflag || kflag) ||
	    linkchain.n == 0 ||
	    linkchain.n > 1 && cflag && outfile)
		usage();

	if (dflag) {
		if ((devnullfd = open("/dev/null", O_WRONLY)) < 0)
			fputs("scc: could not open /dev/null\n", stderr);
	}

	if (!(tmpdir = getenv("TMPDIR")) || !tmpdir[0])
		tmpdir = ".";
	tmpdirln = strlen(tmpdir);

	build(&linkchain, (link = !(Mflag || Eflag || Sflag || cflag)));

	if (!(link || cflag))
		return failure;

	if (link && !failure) {
		spawn(settool(LD, NULL, LAST_TOOL));
		validatetools();
	}

	if (sflag) {
		spawn(settool(inittool(STRIP), NULL, LAST_TOOL));
		validatetools();
	}

	return failure;
}
