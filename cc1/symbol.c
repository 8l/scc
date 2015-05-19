
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "cc1.h"

#define NR_SYM_HASH 32

uint8_t curctx;
static short localcnt;
static short globalcnt;

static Symbol *head;
static Symbol *htab[NR_SYM_HASH];

static inline uint8_t
hash(const char *s)
{
	uint8_t h, ch;

	for (h = 0; ch = *s++; h += ch)
		/* nothing */;
	return h & NR_SYM_HASH - 1;
}

void
pushctx(void)
{
	++curctx;
}

void
popctx(void)
{
	Symbol *next, dummy = {.next = NULL}, *hp = &dummy, *sym;

	if (--curctx == 0)
		localcnt = 0;

	for (sym = head; sym && sym->ctx > curctx; sym = next) {
		next = sym->next;
		if  (sym->ns == NS_LABEL && curctx != 0) {
			hp->next = sym;
			hp = sym;
			continue;
		else if (sym->ns == NS_LABEL && !(sym->flags & ISDEFINED)) {
			/* FIXME: don't recover in this point */
			error("label '%s' is not defined", sym->name);
		} else if (sym->ns == NS_TAG) {
			sym->type->defined = 0;
		}
		htab[hash(sym->name)] = sym->hash;
		free(sym->name);
		free(sym);
	}
	hp->next = sym;
	head = dummy.next;
}

Symbol *
newsym(uint8_t ns)
{
	Symbol *sym;

	sym = malloc(sizeof(*sym));
	sym->ns = ns;
	sym->id = (curctx) ? ++localcnt : ++globalcnt;
	sym->ctx = curctx;
	sym->token = IDEN;
	sym->flags = 0;
	sym->name = NULL;
	sym->type = NULL;
	sym->hash = NULL;
	sym->next = head;
	head = sym;
	return sym;
}

Symbol *
lookup(char *s, uint8_t ns)
{
	Symbol *sym;

	for (sym = htab[hash(s)]; sym; sym = sym->hash) {
		if (!strcmp(sym->name, s) && sym->ns == ns)
			return sym;
	}

	return NULL;
}

Symbol *
install(char *s, uint8_t ns)
{
	Symbol *sym, **t;

	sym = newsym(ns);
	sym->flags |= ISDEFINED;

	if (s) {
		sym->name = xstrdup(s);
		t = &htab[hash(s)];
		sym->hash = *t;
		*t = sym;
	}
	return sym;
}

void
ikeywords(void)
{
	static struct {
		char *str;
		uint8_t token, value;
	} *bp, buff[] = {
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
	};
	Symbol *sym;

	for (bp = buff; bp->str; ++bp) {
		sym = install(bp->str, NS_IDEN);
		sym->token = bp->token;
		sym->u.token = bp->value;
	}
	globalcnt = 0;
}
