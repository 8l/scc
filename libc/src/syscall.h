/* See LICENSE file for copyright and license details. */

extern void *_brk(void *addr);
extern int _open(char *path, int flags, int perm);
extern int _close(int fd);
extern int _read(int fd, void *buf, size_t n);
extern int _write(int fd, void *buf, size_t n);
extern int _lseek(int fd, long off, int whence);
extern void _Exit(int status);
extern int raise(int sig);
extern void (*signal(int sig, void (*func)(int)))(int);
