/* See LICENSE file for copyright and license details. */
#ifndef _STDIO_H
#define _STDIO_H

#include <arch/stdio.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define EOF            -1
#define _IOFBF          0
#define _IOLBF          1
#define _IONBF          2
#define SEEK_CUR        0
#define SEEK_END        1
#define SEEK_SET        2

typedef struct {
	int fd;        	/* file descriptor */
	char flags;     /* bits for must free buffer on close, line-buffered */
	char state;     /* last operation was read, write, position, error, eof */
	char *buf;      /* pointer to i/o buffer */
	char *rp;       /* read pointer (or write end-of-buffer) */
	char *wp;       /* write pointer (or read end-of-buffer) */
	char *lp;       /* actual write pointer used when line-buffering */
	size_t len;    /* actual length of buffer */
	char unbuf[1];  /* tiny buffer for unbuffered io */
} FILE;

extern FILE _IO_stream[FOPEN_MAX];

#define	stderr	(&_IO_stream[2])
#define	stdin	(&_IO_stream[0])
#define	stdout	(&_IO_stream[1])

extern int remove(const char *filename);
extern int rename(const char *old, const char *new);
extern FILE *tmpfile(void);
extern char *tmpnam(char *s);
extern int fclose(FILE *fp);
extern int fflush(FILE *fp);
extern FILE *fopen(const char * restrict fname, const char * restrict mode);
extern FILE *freopen(const char * restrict fname, const char * restrict mode,
                     FILE * restrict fp);
extern void setbuf(FILE * restrict fp, char * restrict buf);
extern int setvbuf(FILE * restrict fp,
                   char * restrict buf, int mode, size_t size);
extern int fprintf(FILE * restrict fp, const char * restrict fmt, ...);
extern int fscanf(FILE * restrict fp, const char * restrict fmt, ...);
extern int printf(const char * restrict fmt, ...);
extern int scanf(const char * restrict fmt, ...);
extern int snprintf(char * restrict s,
                    size_t n, const char * restrict fmt, ...);
extern int sprintf(char * restrict s, const char * restrict fmt, ...);
extern int sscanf(const char * restrict s, const char * restrict fmt, ...);

#ifdef _STDARG_H
extern int vfprintf(FILE * restrict fp,
                    const char * restrict fmt, va_list arg);
extern int vfscanf(FILE * restrict fp,
                   const char * restrict fmt, va_list arg);
extern int vprintf(const char * restrict fmt, va_list arg);
extern int vscanf(const char * restrict fmt, va_list arg);
extern int vsnprintf(char * restrict s, size_t n, const char * restrict fmt,
                     va_list arg);
extern int vsprintf(char * restrict s,
                    const char * restrict fmt, va_list arg);
extern int vsscanf(const char * restrict s,
                   const char * restrict fmt, va_list arg);
#endif

extern int fgetc(FILE *fp);
extern char *fgets(char * restrict s, int n, FILE * restrict fp);
extern int fputc(int c, FILE *fp);
extern int fputs(const char * restrict s, FILE * restrict fp);
extern int getc(FILE *fp);
extern int getchar(void);
extern char *gets(char *s);
extern int putc(int c, FILE *fp);
extern int putchar(int c);
extern int puts(const char *s);
extern int ungetc(int c, FILE *fp);
extern size_t fread(void * restrict ptr, size_t size, size_t nmemb,
             FILE * restrict fp);
extern size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
              FILE * restrict fp);
extern int fgetpos(FILE * restrict fp, fpos_t * restrict pos);
extern int fseek(FILE *fp, long int offset, int whence);
extern int fsetpos(FILE *fp, const fpos_t *pos);
extern long int ftell(FILE *fp);
extern void rewind(FILE *fp);
extern void clearerr(FILE *fp);
extern int feof(FILE *fp);
extern int ferror(FILE *fp);
extern void perror(const char *s);

#ifdef __USE_MACROS
#define printf(...) fprintf(stdout, __VA_ARGS__)
#endif

#endif
