#ifndef CC_H
#define CC_H

extern unsigned linenum;
extern unsigned columnum;
extern const char *filename;


struct user_opt {
	unsigned char implicit_int;
	unsigned char c99;
	unsigned char useless_typename;
};


extern  struct user_opt user_opt;

extern void warning(const char *fmt, ...);
extern void error(const char *fmt, ...);
extern void die(const char *fmt, ...);
extern void warning_error(char flag, const char *fmt, ...);
#endif
