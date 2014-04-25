
#ifndef CC_H_
#define CC_H_

#ifndef __bool_true_and_false_defined
#include <stdbool.h>
#endif

extern void die(const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);

#endif