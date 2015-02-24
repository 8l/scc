
#ifndef CC_H_
#define CC_H_

#ifndef __bool_true_and_false_defined
#ifdef NBOOL
typedef unsigned bool;
#else
#include <stdbool.h>
#endif
#endif

#define TINT short

#define RANK_BOOL    0
#define RANK_SCHAR   1
#define RANK_UCHAR   2
#define RANK_CHAR    3
#define RANK_SHORT   4
#define RANK_USHORT  5
#define RANK_INT     6
#define RANK_UINT    7
#define RANK_LONG    8
#define RANK_ULONG   9
#define RANK_LLONG   10
#define RANK_ULLONG  11
#define RANK_FLOAT   12
#define RANK_DOUBLE  13
#define RANK_LDOUBLE 15

#define L_INT8      'C'
#define L_INT16     'I'
#define L_INT32     'L'
#define L_INT64     'Q'
#define L_UINT8     'M'
#define L_UINT16    'N'
#define L_UINT32    'Z'
#define L_UINT64    'O'

#define L_VOID      '0'
#define L_POINTER   'P'
#define L_FUNCTION  'F'
#define L_ARRAY     'V'
#define L_UNION     'U'
#define L_STRUCT    'S'

#define L_SCHAR     L_INT8
#define L_UCHAR     L_UINT8
#define L_CHAR      L_UINT8
#define L_SHORT     L_INT16
#define L_USHORT    L_UINT16
#define L_INT       L_INT16
#define L_UINT      L_UINT16
#define L_LONG      L_INT32
#define L_ULONG     L_UINT32
#define L_LLONG     L_INT64
#define L_ULLONG    L_UINT64
#define L_BOOL      'B'
#define L_FLOAT     'J'
#define L_DOUBLE    'D'
#define L_LDOUBLE   'H'

extern void die(const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
extern void *xrealloc(void *buff, register size_t size);

#endif
