#ifndef SYNTAX_H
#define SYNTAX_H

extern unsigned char nested_level;

extern void compound(void);
extern void expr(void);
extern unsigned char decl(void);
extern void type_name(void);
#endif
