
#ifndef __bool_true_and_false_defined
#ifdef NBOOL
typedef unsigned bool;
#else
#include <stdbool.h>
#endif
#endif

#ifndef NDEBUG
extern int debug;
#define DBG(fmt, ...) dbg(fmt, __VA_ARGS__)
#define DBGON() (debug = 1)
#else
#define DBG(...)
#define DBGON()
#endif

#define L_INT8      'C'
#define L_INT16     'I'
#define L_INT32     'W'
#define L_INT64     'Q'
#define L_UINT8     'K'
#define L_UINT16    'N'
#define L_UINT32    'Z'
#define L_UINT64    'O'
#define L_ELLIPSIS  'E'

#define L_VOID      '0'
#define L_POINTER   'P'
#define L_FUNCTION  'F'
#define L_ARRAY     'V'
#define L_UNION     'U'
#define L_STRUCT    'S'

#define L_PUBLIC    'G'
#define L_PRIVATE   'Y'
#define L_LOCAL     'T'
#define L_REGISTER  'R'
#define L_FIELD     'M'
#define L_AUTO      'A'
#define L_EXTERN    'X'

extern void die(const char *fmt, ...);
extern void dbg(const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);
