
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
extern struct symhash *siden, *sgoto, *sstruct;

struct symbol *
addsym(struct symhash *h, struct symbol *sym, unsigned char hash);

#endif
