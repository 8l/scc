/* See LICENSE file for copyright and license details. */
#include <stddef.h>

extern int debug;

#ifndef NDEBUG
#define DBG(...) dbg(__VA_ARGS__)
#define DBGON() (debug = 1)
#else
#define DBG(...)
#define DBGON()
#endif

#ifndef PREFIX
#define PREFIX "/usr/local/"
#endif

#define TINT        long long
#define TUINT       unsigned long long
#define TFLOAT      double

struct items {
	char **s;
	unsigned n;
};

extern void die(const char *fmt, ...);
extern void dbg(const char *fmt, ...);
extern void newitem(struct items *items, char *item);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);
