
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"
#include "cc1.h"

#define NR_SYM_HASH 64

static unsigned curctx;
static short localcnt;
static short globalcnt;

static Symbol *head;
static Symbol *htab[NR_SYM_HASH];

static inline unsigned
hash(const char *s)
{
	unsigned c, h;

	for (h = 0; c = *s; ++s)
		h ^= 33 * c;
	return h & NR_SYM_HASH-1;
}

void
pushctx(void)
{
	if (++curctx == NR_BLOCK)
		error("too much nested blocks");
}

void
popctx(void)
{
	Symbol *next, dummy = {.next = NULL}, *hp = &dummy, *sym;

	if (--curctx == 0)
		localcnt = 0;

	for (sym = head; sym && sym->ctx > curctx; sym = next) {
		next = sym->next;
		if  (sym->ns == NS_CPP || sym->ns == NS_LABEL && curctx != 0) {
			hp->next = sym;
			hp = sym;
			continue;
		} else if (sym->ns == NS_LABEL && !(sym->flags & ISDEFINED)) {
			/* FIXME: don't recover in this point */
			error("label '%s' is not defined", sym->name);
		} else if (sym->ns == NS_TAG) {
			sym->type->defined = 0;
		}
		if (sym->hash)
			htab[hash(sym->name)] = sym->hash;
		free(sym->name);
		free(sym);
	}
	hp->next = sym;
	head = dummy.next;
}

Symbol *
newsym(unsigned ns)
{
	Symbol *sym;

	sym = malloc(sizeof(*sym));
	sym->ns = ns;
	sym->id = (curctx) ? ++localcnt : ++globalcnt;
	sym->ctx = curctx;
	sym->token = IDEN;
	sym->flags = ISDEFINED;
	sym->name = NULL;
	sym->type = NULL;
	sym->hash = NULL;
	sym->next = head;
	head = sym;
	return sym;
}

Symbol *
lookup(unsigned ns)
{
	Symbol *sym, **h;
	unsigned sns;
	char *t, c;

	h = &htab[hash(yytext)];
	c = *yytext;
	for (sym = *h; sym; sym = sym->hash) {
		t = sym->name;
		if (*t != c || strcmp(t, yytext))
			continue;
		sns = sym->ns;
		if (sns == NS_KEYWORD || sns == NS_CPP)
			return sym;
		if (sns != ns)
			continue;
		return sym;
	}

	sym = newsym(ns);
	sym->name = xstrdup(yytext);
	sym->flags &= ~ISDEFINED;
	sym->hash = *h;
	*h = sym;
	return sym;
}

Symbol *
install(unsigned ns)
{
	Symbol *sym, **h;
	/*
	 * install() is always called after a call to lookup(), so
	 * yylval.sym always points to a symbol with yytext name.
	 * if the symbol is an undefined symbol and in the same
	 * context, then it was generated in the previous lookup()
	 * call. If the symbol is defined and in the same context
	 * then there is a redefinition
	 */
	if (yylval.sym->ctx == curctx) {
		if (yylval.sym->flags & ISDEFINED)
			return NULL;
		yylval.sym->flags |= ISDEFINED;
		return yylval.sym;
	}

	sym = newsym(ns);
	sym->name = xstrdup(yytext);

	if (yylval.sym->ns == NS_CPP) {
		sym->hash = yylval.sym->hash;
		yylval.sym->hash = sym;
		return sym;
	}

	h = &htab[hash(yytext)];
	sym->hash = *h;
	*h = sym;
	return sym;
}

void
ikeywords(void)
{
	static struct {
		char *str;
		unsigned char token, value;
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
		strcpy(yytext, bp->str);
		sym = lookup(NS_KEYWORD);
		sym->token = bp->token;
		sym->u.token = bp->value;
	}
	globalcnt = 0;
}
