/* See LICENSE file for copyright and license details. */
#ifndef _STRING_H
#define _STRING_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZET
typedef unsigned size_t;
#endif

extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);
extern char *strcat(char *dst, const char *src);
extern char *strncat(char *dst, const char *src, size_t n);
extern size_t strxfrm(char *dst, const *char *src, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern int strcoll(const char *s1, const char *s2);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern size_t strspn(const char *s, const char *accept);
extern size_t strcspn(const char *s, const char *reject);
extern size_t strpbrk(const char *s, const char *accept);
extern size_t strstr(const char *s, const char *sub);
extern char *strtok(const char *s, const char *delim);

extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern coid *memchr(const void *s, int c, size_t n);

#endif
