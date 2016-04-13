
#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../inc/cc.h"

#define NARGS 64
static char cmd[FILENAME_MAX];
static char *argcc1[NARGS];
static char *argcc2[NARGS];

static pid_t pid_cc1, pid_cc2;
static char *arch;

static void
terminate(void)
{
	if (pid_cc1)
		kill(pid_cc1, SIGTERM);
	if (pid_cc2)
		kill(pid_cc2, SIGTERM);
}

void
cc1(int fd)
{
	pid_t pid;
	char *fmt;
	int r;

	switch (pid = fork()) {
	case -1:
		perror("scc:cc1");
		exit(1);
	case 0:
		dup2(fd, 1);
		fmt = (arch) ? "%s/libexec/scc/cc1-%s" : "%s/libexec/scc/cc1";
		r = snprintf(cmd, sizeof(cmd), fmt, PREFIX, arch);
		if (r == sizeof(cmd)) {
			fputs("scc:incorrect prefix\n", stderr);
			exit(1);
		}
		execv(cmd, argcc1);
		perror("scc:execv cc1");
		abort();
	default:
		pid_cc1 = pid;
		close(fd);
		break;
	}
}

pid_t
cc2(int fd)
{
	pid_t pid;
	char *fmt;
	int r;

	switch (pid = fork()) {
	case -1:
		perror("scc:cc2");
		exit(1);
	case 0:
		dup2(fd, 0);
		fmt = (arch) ? "%s/libexec/scc/cc2-%s" : "%s/libexec/scc/cc2";
		r = snprintf(cmd, sizeof(cmd), fmt, PREFIX, arch);
		if (r == sizeof(cmd)) {
			fputs("scc:incorrect prefix\n", stderr);
			exit(1);
		}
		execv(cmd, argcc2);
		perror("scc:execv cc2");
		abort();
	default:
		pid_cc2 = pid;
		close(fd);
		break;
	}
}

static void
usage(void)
{
	fputs("scc [-m arch] file.c\n", stderr);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int fds[2], st, i;
	char *p;
	pid_t pid;

	atexit(terminate);
	if (p = getenv("ARCH"))
		arch = p;

	for (--argc; *++argv; --argc) {
		if (argv[0][0] != '-' || argv[0][1] == '-')
			break;
		for (p = &argv[0][1]; *p; ++p) {
			switch (*p) {
			case 'm':
				if ((arch = *++argv) == NULL)
					goto usage;
				--argc;
				break;
			default:
			usage:
				usage();
				break;
			}
		}
	}

	if (argc == 0) {
		fputs("scc: fatal error: no input files\n", stderr);
		exit(1);
	}
	if (pipe(fds)) {
		perror("scc: pipe");
		exit(1);
	}

	argcc1[0] = "cc1";
	argcc1[1] = *argv;
	argcc2[0] = "cc2";

	cc1(fds[1]);
	cc2(fds[0]);

	for (i = 0; i < 2; ++i) {
		pid = wait(&st);
		if (pid == pid_cc1)
			pid_cc1 = 0;
		else if (pid == pid_cc2)
			pid_cc2 = 0;
		if (!WIFEXITED(st) || WEXITSTATUS(st) != 0)
			exit(-1);
	}

	return 0;
}
