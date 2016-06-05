/* See LICENSE file for copyright and license details. */
#ifndef _STDIO_H
#define _STDIO_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZET
typedef unsigned size_t;
#endif

#define EOF            -1
#define BUFSIZ        512
#define FILENAME_MAX  256
#define FOPEN_MAX      16
#define _IOFBF          0
#define _IOLBF          1
#define _IONBF          2
#define L_tmpnam      256
#define SEEK_CUR        0
#define SEEK_END        1
#define SEEK_SET        2
#define TMP_MAX        25

typedef struct _FILE FILE;

extern FILE *fopen(const char *, const char *mode);
extern FILE *freopen(const char *path, const char *mode, FILE *fp);
extern int fclose(FILE *fp);

extern int fflush(FILE *fp);
extern void setbuf(FILE *fp, char *buf);
extern int setvbuf(FILE *fp, char *buf, size_t size);

extern size_t fread(void *ptr, size_t size, size_t n, FILE *fp);
extern size_t fwrite(const void *ptr, size_t size, size n, FILE *fp);

extern int fgetc(FILE *fp);
extern int getc(FILE *fp);
extern int getchar(void);

extern int fputc(int c, FILE *fp);
extern int putc(int c, FILE *fp);
extern int putchar(int c);

extern char *fgets(char *s, int size, FILE *fp);
extern char *gets(char *s);

extern int fputs(char *s, FILE *fp);
extern int puts(char *s);

extern int scanf(const char *fmt, ...);
extern int fscanf(FILE *fp, const char *fmt, ...);
extern int sscanf(char *s, const char *fmt, ...);

extern int printf(const char *fmt, ...);
extern int fprintf(FILE *fp, const char *fmt, ...);
extern int sprintf(char *s, const char *fmt, ...);
extern int snprintf(char *s, size_t size, const char *fmt, ...);

extern void perror(const char *s);

extern long ftell(FILE *fp);
extern long fseek(FILE *fp);
extern void rewind(FILE *fp);

extern void clearerr(FILE *fp);
extern int feof(FILE *fp);
extern int ferror(FILE *fp);

extern int remove(const char *name);
extern int rename(const char *old, const char *new);
extern FILE *tmpfile(void);
extern FILE *tmpnam(char *s);

extern FILE *stdin, *stdio, *stderr;
#endif
