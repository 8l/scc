/* See LICENSE file for copyright and license details. */
#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/arg.h"
#include "../../inc/cc.h"

#define NARGS 64

enum {
	CC1,
	CC2,
	QBE,
	AS,
	LD,
	TEE,
	NR_TOOLS,
};

static struct tool {
	char  cmd[PATH_MAX];
	char *args[NARGS];
	char  bin[16];
	char *outfile;
	int   nargs, in, out;
	pid_t pid;
} tools[NR_TOOLS] = {
	[CC1] = { .bin = "cc1", .cmd = PREFIX "/libexec/scc/", },
	[CC2] = { .bin = "cc2", .cmd = PREFIX "/libexec/scc/", },
	[QBE] = { .bin = "qbe", .cmd = "qbe", },
	[AS]  = { .bin = "as",  .cmd = "as", },
	[LD]  = { .bin = "gcc", .cmd = "gcc", }, /* TODO replace with ld */
	[TEE] = { .bin = "tee", .cmd = "tee", },
};

char *argv0;
static char *arch;
static char *tmpobjs[NARGS - 2];
static int nobjs;
static int failedtool = NR_TOOLS;
static int Eflag, Sflag, kflag;

static void
terminate(void)
{
	struct tool *t;
	int i;

	for (i = 0; i < NR_TOOLS; ++i) {
		t = &tools[i];
		if (t->pid)
			kill(t->pid, SIGTERM);
		if (i >= failedtool && t->outfile)
			unlink(t->outfile);
	}
}

static int
inittool(int tool)
{
	struct tool *t = &tools[tool];
	size_t binln;
	int n;

	if (!t->args[0]) {
		switch (tool) {
		case CC1:
		case CC2:
			binln = strlen(t->bin);
			if (arch) {
				n = snprintf(t->bin + binln,
					     sizeof(t->bin) - binln,
					     "-%s", arch);
				if (n < 0 || n >= sizeof(t->bin))
					die("scc: target tool bin too long");
				binln = strlen(t->bin);
			}

			if (strlen(t->cmd) + binln + 1 > sizeof(t->cmd))
				die("scc: target tool path too long");
			strcat(t->cmd, t->bin);
			break;
		case AS:
			t->nargs = 2;
			t->args[1] = "-o";
			break;
		case LD:
			t->nargs = 2;
			t->args[1] = "-o";
			break;
		default:
			break;
		}

		t->args[0] = t->bin;
	}

	return tool;
}

static char *
outfilename(char *path, char *ext)
{
	char *new, *name, *dot;
	size_t newsz, nameln;
	int n;

	if (!(name = strrchr(path, '/')))
		name = path;
	else
		++name;

	nameln = strlen(name);

	if (!(dot = strrchr(name, '.')))
		dot = &name[nameln];

	nameln = nameln - strlen(dot);
	newsz  = nameln + strlen(ext) + 1 + 1;

	new = xmalloc(newsz);

	n = snprintf(new, newsz, "%.*s.%s", nameln, name, ext);
	if (n < 0 || n >= newsz)
		die("scc: wrong output filename");

	return new;
}

static void
addarg(int tool, char *arg) {
	struct tool *t = &tools[tool];

	if (!(t->nargs < NARGS - 2)) /* 2: argv0, NULL terminator */
		die("scc: too many parameters given");

	t->args[++t->nargs] = arg;
}

static int
settool(int tool, char *input, int nexttool)
{
	struct tool *t = &tools[tool];
	int fds[2], proxiedtool;
	char *ext;
	static int fdin;

	switch (tool) {
	case AS:
		t->outfile = outfilename(input, "o");
		t->args[2] = t->outfile;
		break;
	case LD:
		if (!t->outfile) {
			t->outfile = "a.out";
			t->args[2] = t->outfile;
		}
		break;
	case TEE:
		switch (nexttool) {
		case CC2:
			proxiedtool = CC1;
			ext = "ir"; break;
		case QBE:
			proxiedtool = CC2;
			ext = "qbe"; break;
		case NR_TOOLS:
		case AS:
			proxiedtool = CC2;
			ext = "as"; break;
		}
		tools[proxiedtool].outfile = outfilename(input, ext);
		t->args[1] = tools[proxiedtool].outfile;
		break;
	default:
		break;
	}

	if (fdin) {
		t->in = fdin;
		fdin = 0;
	} else {
		t->args[t->nargs + 1] = input;
	}

	if (nexttool < NR_TOOLS && nexttool != LD) {
		if (pipe(fds))
			die("scc: pipe: %s", strerror(errno));
		t->out = fds[1];
		fdin = fds[0];
	}

	return tool;
}

static void
spawn(int t)
{
	struct tool *tool = &tools[t];

	switch (tool->pid = fork()) {
	case -1:
		die("scc: %s: %s", tool->bin, strerror(errno));
	case 0:
		if (tool->out)
			dup2(tool->out, 1);
		if (tool->in)
			dup2(tool->in, 0);
		execvp(tool->cmd, tool->args);
		fprintf(stderr, "scc: execp %s: %s\n",
		        tool->cmd, strerror(errno));
		_exit(1);
	default:
		if (tool->in)
			close(tool->in);
		if (tool->out)
			close(tool->out);
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
		if (!strcmp(dot, ".as"))
			return AS;
	}

	die("scc: do not recognize filetype of %s", file);
}

static void
checktool(int tool)
{
	struct tool *t = &tools[tool];
	int st;

	if (!t->pid)
		return;

	if (waitpid(t->pid, &st, 0) < 0 ||
	    !WIFEXITED(st) || WEXITSTATUS(st) != 0) {
		failedtool = tool;
		exit(-1);
	}

	t->pid = 0;
}

static void
linkobjs(void)
{
	int i;

	settool(inittool(LD), NULL, NR_TOOLS);

	for (i = 0; tmpobjs[i] && i < nobjs; ++i)
		addarg(LD, tmpobjs[i]);

	spawn(LD);

	checktool(LD);

	if (!kflag) {
		for (i = 0; i < nobjs; ++i)
			unlink(tmpobjs[i]);
	}

	return;
}

static void
build(char *file)
{
	int i, tool, nexttool, keepfile;
	int backtool;

	for (tool = toolfor(file); tool < NR_TOOLS; tool = nexttool) {
		keepfile = 0;

		switch (tool) {
		case CC1:
			nexttool = Eflag ? NR_TOOLS : CC2;
			if (!Eflag)
				keepfile = kflag;
			break;
		case CC2:
			if (!arch || strcmp(arch, "qbe")) {
				nexttool = Sflag ? NR_TOOLS : AS;
				keepfile = (Sflag || kflag);
			} else {
				nexttool = QBE;
				keepfile = kflag;
			}
			break;
		case QBE:
			nexttool = Sflag ? NR_TOOLS : AS;
			keepfile = (Sflag || kflag);
			break;
		case AS:
			backtool = AS;
			nexttool = LD;
			break;
		case LD:
			if (backtool == AS)
				tmpobjs[nobjs++] = xstrdup(tools[AS].outfile);
			else
				addarg(LD, file);
			nexttool = NR_TOOLS;
			continue;
		case TEE:
			nexttool = backtool;
			break;
		default:
			break;
		}

		if (keepfile) {
			backtool = nexttool;
			nexttool = TEE;
		}

		spawn(settool(inittool(tool), file, nexttool));
	}

	for (i = 0; i < NR_TOOLS; ++i)
		checktool(i);

	for (i = 0; i < NR_TOOLS; ++i) {
		if (i != LD) {
			free(tools[i].outfile);
			tools[i].outfile = NULL;
		}
	}
}

static void
usage(void)
{
	die("usage: %s [-E|-kS] [-m arch] [-D macro[=val]]... "
	    "[-I dir]... file ...", argv0);
}

int
main(int argc, char *argv[])
{
	atexit(terminate);

	arch = getenv("ARCH");

	ARGBEGIN {
	case 'D':
		addarg(CC1, "-D");
		addarg(CC1, EARGF(usage()));
		break;
	case 'E':
		Eflag = 1;
		addarg(CC1, "-E");
		break;
	case 'I':
		addarg(CC1, "-I");
		addarg(CC1, EARGF(usage()));
		break;
	case 'S':
		Sflag = 1;
		break;
	case 'k':
		kflag = 1;
		break;
	case 'm':
		arch = EARGF(usage());
		break;
	case '-':
		fprintf(stderr,
		        "scc: ignored parameter --%s\n", EARGF(usage()));
		break;
	default:
		usage();
	} ARGEND

	if (Eflag && (Sflag || kflag))
		usage();

	if (!argc)
		die("scc: fatal error: no input file");

	for (; *argv; ++argv)
		build(*argv);

	if (!(Eflag || Sflag))
		linkobjs();

	return 0;
}
