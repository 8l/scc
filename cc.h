#ifndef CC_H
#define CC_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

extern unsigned linenum;
extern unsigned columnum;
extern const char *filename;


struct user_opt {
	unsigned char implicit;
	unsigned char c99;
	unsigned char useless;
	unsigned char repeat;
};

extern  struct user_opt options;

extern void warning(const char *fmt, ...);
extern void error(const char *fmt, ...);
extern void die(const char *fmt, ...);
extern void warning_error(char flag, const char *fmt, ...);
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);
#endif
