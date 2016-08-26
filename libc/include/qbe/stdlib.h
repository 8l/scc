/* See LICENSE file for copyright and license details. */
#ifndef _STDLIB_H
#define _STDLIB_H

#ifndef _SIZET
typedef unsigned long size_t;
#define _SIZET
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define RAND_MAX     32767

extern double atof(const char *ptr);
extern int atoi(const char *s);
extern long atol(const char *s);
extern long long atoll(const char *s);

extern float strtof(const char *s, char **end);
extern double strtod(const char *s, char **end);
extern long double strtold(const char *s, char **end);

extern long strtol(const char *s, char **end, int base);
extern long long strtoll(const char *s, char **end, int base);
extern unsigned long stroul(const char *s, char **end, int base);
extern unsigned long long stroull(const char *s, char **end, int base);

extern void *calloc(size_t nitems, size_t size);
extern void free(void *ptr);
extern void *realloc(void *ptr, size_t size);

extern void abort(void);
extern int atexit(void (*func)(void));
extern void exit(int status);
extern char *getenv(const char *name);
extern int system(const char *cmd);

extern void *bsearch(const void *key,
                     const void *base, size_t nitems, size_t size,
                     int (*cmp)(const void *, const void *));
extern void qsort(void *base, size_t nitems, size_t size,
                  int (*cmp)(const void *, const void *));

extern void abs(int x);
/* div_t div(int num, int denom); */
extern long labs(long int x);
/* ldiv_t ldiv(long int number, long int denom); */

extern int rand(void);
extern void srand(unsigned seed);

#endif
