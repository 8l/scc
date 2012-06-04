#ifndef CC_H
#define CC_H

extern unsigned linenum;
extern unsigned columnum;
extern const char *filename;

struct {
	unsigned implicit_int : 1;
	unsigned c99 : 1;
	unsigned useless_typename : 1;
} user_opt;


extern void warning(const char *fmt, ...);
extern void error(const char *fmt, ...);
extern void die(const char *fmt, ...);
extern void warning_error(char flag, const char *fmt, ...);
#endif
