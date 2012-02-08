
#pragma once
#ifndef SYMBOL_H
#define SYMBOL_H


struct type;

struct symbol {
	char *str;
	struct type *type;
	struct symbol *next;
};


struct symhash;
extern struct symhash siden, sgoto, sstruct;

extern unsigned char hashfun(register const char *s);


#endif
