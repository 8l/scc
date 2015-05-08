
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

#define NR_SYM_HASH 32

uint8_t curctx;
static short localcnt;
static short globalcnt;

static struct symtab {
	Symbol *head;
	Symbol *htab[NR_SYM_HASH];
} symtab [NR_NAMESPACES];

static inline uint8_t
hash(const char *s)
{
	uint8_t h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h & NR_SYM_HASH - 1;
}

static void
freesyms(uint8_t ns)
{
	static struct symtab *tbl;
	Symbol *sym, *next;

	tbl = &symtab[ns];
	for (sym = tbl->head; sym; sym = next) {
		if (sym->ctx <= curctx)
			break;
		if (ns == NS_LABEL && !sym->isdefined)
			error("label '%s' is not defined", sym->name);
		if (ns == NS_TAG)
			sym->type->defined = 0;
		tbl->htab[hash(sym->name)] = sym->hash;
		next = tbl->head = sym->next;
		free(sym->name);
		free(sym);
	}
}

void
pushctx(void)
{
	++curctx;
}

void
popctx(void)
{
	--curctx;
	freesyms(NS_IDEN);
	freesyms(NS_TAG);
	freesyms(NS_STRUCTS);
	if (curctx == 0) {
		localcnt = 0;
		freesyms(NS_LABEL);
	}
}

Symbol *
lookup(char *s, uint8_t ns)
{
	struct symtab *tbl;
	Symbol *sym;

	tbl = &symtab[(ns > NS_STRUCTS) ? NS_STRUCTS : ns];
	for (sym = tbl->htab[hash(s)]; sym; sym = sym->hash) {
		if (!strcmp(sym->name, s) && sym->ns == ns)
			return sym;
	}

	return NULL;
}

Symbol *
install(char *s, uint8_t ns)
{
	Symbol *sym, **t;
	struct symtab *tbl;

	sym = xcalloc(1, sizeof(*sym));
	sym->name = xstrdup(s);
	sym->ctx = curctx;
	sym->token = IDEN;
	sym->id = (curctx) ? ++localcnt : ++globalcnt;
	sym->isdefined = 1;
	sym->ns = ns;
	tbl = &symtab[(ns > NS_STRUCTS) ? NS_STRUCTS : ns];
	sym->next = tbl->head;
	tbl->head = sym;

	t = &tbl->htab[hash(s)];
	sym->hash = *t;
	return *t = sym;
}

void
ikeywords(void)
{
	extern void include(char *), define(char *), undef(char *);
	extern void ifdef(char *),  ifndef(char *), endif(char *);
	static struct words {
		char *str;
		uint8_t token, value;
		void (*fun)(char *);
	} ccwords[] = {
		{"auto", SCLASS, AUTO},
		{"break", BREAK, BREAK},
		{"_Bool", TYPE, BOOL},
		{"case", CASE, CASE},
		{"char", TYPE, CHAR},
		{"const", TQUALIFIER, CONST},
		{"continue", CONTINUE, CONTINUE},
		{"default", DEFAULT, DEFAULT},
		{"do", DO, DO},
		{"double", TYPE, DOUBLE},
		{"else", ELSE, ELSE},
		{"enum", TYPE, ENUM},
		{"extern", SCLASS, EXTERN},
		{"float", TYPE, FLOAT},
		{"for", FOR, FOR},
		{"goto", GOTO, GOTO},
		{"if", IF, IF},
		{"int", TYPE, INT},
		{"long", TYPE, LONG},
		{"register", SCLASS, REGISTER},
		{"restrict", TQUALIFIER, RESTRICT},
		{"return", RETURN, RETURN},
		{"short", TYPE, SHORT},
		{"signed", TYPE, SIGNED},
		{"sizeof", SIZEOF, SIZEOF},
		{"static", SCLASS, STATIC},
		{"struct", TYPE, STRUCT},
		{"switch", SWITCH, SWITCH},
		{"typedef", SCLASS, TYPEDEF},
		{"union", TYPE, UNION},
		{"unsigned", TYPE, UNSIGNED},
		{"void", TYPE, VOID},
		{"volatile", TQUALIFIER, VOLATILE},
		{"while", WHILE, WHILE},
		{NULL, 0, 0},
	}, cppwords[] = {
		{"include", 0, 0, include},
		{"define", 0, 0, define},
		{"undef", 0, 0, undef},
		{"ifdef", 0, 0, ifdef},
		{"ifndef", 0, 0, ifndef},
		{"endif", 0, 0, endif},
		{NULL, 0, 0}
	};
	static struct wordlist {
		struct words *words;
		uint8_t ns;
	} wordlist [] = {
		{ccwords, NS_IDEN},
		{cppwords, NS_CPP},
		{NULL, 0}
	};
	struct wordlist *lp;
	struct words *bp;
	Symbol *sym;

	for (lp = wordlist; lp->words; ++lp) {
		for (bp = lp->words; bp->str; ++bp) {
			sym = install(bp->str, lp->ns);
			sym->token = bp->token;
			if (bp->fun)
				sym->u.fun = bp->fun;
			else
				sym->u.token = bp->value;
		}
	}
	globalcnt = 0;
}
