/* See LICENSE file for copyright and license details. */

typedef char sig_atomic_t;

#define SIG_ERR    -1
#define SIG_DFL     0
#define SIG_IGN     1

#define SIG_ERR    -1
#define SIG_DFL     0
#define SIG_IGN     1

#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGABRT     6
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1    10
#define SIGSEGV    11
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
#define SIGTERM    15
#define SIGCHLD    17
#define SIGCONT    18
#define SIGSTOP    19
#define SIGSSTP    20
#define SIGTTIN    21
#define SIGTTOU    22

#define __NR_SIGNALS 23
