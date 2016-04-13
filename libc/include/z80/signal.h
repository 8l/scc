#ifndef _SIGNAL_H
#define _SIGNAL_H

void ( *signal(int signum, void (*handler)(int)) ) (int);
int raise(int sig);

#endif