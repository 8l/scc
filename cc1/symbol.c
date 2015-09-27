
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"
#include "cc1.h"

#define NR_SYM_HASH 64

unsigned curctx;
static unsigned short counterid;

static Symbol *head, *labels;
static Symbol *htab[NR_SYM_HASH];

#ifndef NDEBUG
void
dumpstab(char *msg)
{
	Symbol **bp, *sym;

	fprintf(stderr, "Symbol Table dump at ctx=%u\n%s\n", curctx, msg);
	for (bp = htab; bp < &htab[NR_SYM_HASH]; ++bp) {
		if (*bp == NULL)
			continue;
		fprintf(stderr, "%d", (int) (bp - htab));
		for (sym = *bp; sym; sym = sym->hash)
			fprintf(stderr, "->[%d,%d:'%s'=%p]",
			        sym->ns, sym->ctx, sym->name, (void *) sym);
		putc('\n', stderr);
	}
	fputs("head:", stderr);
	for (sym = head; sym; sym = sym->next) {
		fprintf(stderr, "->[%d,%d:'%s'=%p]",
		        sym->ns, sym->ctx,
		        (sym->name) ? sym->name : "", (void *) sym);
	}
	putc('\n', stderr);
}
#endif

static unsigned
hash(const char *s)
{
	unsigned c, h;

	for (h = 0; c = *s; ++s)
		h ^= 33 * c;
	return h & NR_SYM_HASH-1;
}

static void
unlinkhash(Symbol *sym)
{
	Symbol **h, *p, *prev;

	if ((sym->flags & ISDECLARED) == 0)
		return;
	h = &htab[hash(sym->name)];
	for (prev = p = *h; p != sym; prev = p, p = p->hash)
		/* nothing */;
	if (prev == p)
		*h = sym->hash;
	else
		prev->hash = sym->hash;
}

void
pushctx(void)
{
	if (++curctx == NR_BLOCK+1)
		error("too much nested blocks");
}

static void
killsym(Symbol *sym)
{
	short f;
	char *name;

	f = sym->flags;
	if (f & ISSTRING)
		free(sym->u.s);
	if (sym->ns == NS_TAG)
		sym->type->defined = 0;
	if ((name = sym->name) != NULL) {
		unlinkhash(sym);
		if ((f & (ISUSED|ISGLOBAL|ISDECLARED)) == ISDECLARED)
			warn("'%s' defined but not used", name);
		if ((f & ISDEFINED) == 0 && sym->ns == NS_LABEL)
			errorp("label '%s' is not defined", name);
		free(name);
	}
	free(sym);
}

void
popctx(void)
{
	Symbol *next, *sym;
	char *name;
	short f;

	if (--curctx == GLOBALCTX) {
		for (sym = labels; sym; sym = next) {
			next = sym->next;
			killsym(sym);
		}
		labels = NULL;
	}

	for (sym = head; sym && sym->ctx > curctx; sym = next) {
		next = sym->next;
		killsym(sym);
	}
	head = sym;
}

static unsigned short
newid(void)
{
	unsigned short id;

	id = ++counterid;
	if (id == 0) {
		die("Overflow in %s identifiers",
		    (curctx) ? "internal" : "external");
	}
	return id;
}

Type *
duptype(Type *base)
{
	Type *tp = xmalloc(sizeof(*tp));

	*tp = *base;
	tp->id = newid();
	return tp;
}

static Symbol *
allocsym(int ns, char *name)
{
	Symbol *sym;

	sym = xmalloc(sizeof(*sym));
	if (name)
		name = xstrdup(name);
	sym->name = name;
	sym->id = 0;
	sym->ns = ns;
	sym->ctx = (ns == NS_CPP) ? UCHAR_MAX : curctx;
	sym->token = IDEN;
	sym->flags = 0;
	sym->u.s = NULL;
	sym->type = NULL;
	sym->next = sym->hash = NULL;
	return sym;
}

static Symbol *
linksym(Symbol *sym)
{
	Symbol *p, *prev;

	sym->flags |= ISDECLARED;
	switch (sym->ns) {
	case NS_CPP:
		return sym;
	case NS_LABEL:
		sym->next = labels;
		return labels = sym;
	default:
		for (prev = p = head; p; prev = p, p = p->next) {
			if (p->ctx <= sym->ctx)
				break;
		}
		if (p == prev) {
			sym->next = head;
			head = sym;
		} else {
			p = prev->next;
			prev->next = sym;
			sym->next = p;
		}
		return sym;
	}
}

static Symbol *
linkhash(Symbol *sym)
{
	Symbol **h, *p, *prev;

	h = &htab[hash(sym->name)];

	for (prev = p = *h; p; prev = p, p = p->hash) {
		if (p->ctx <= sym->ctx)
			break;
	}
	if (p == prev) {
		sym->hash = *h;
		*h = sym;
	} else {
		p = prev->hash;
		prev->hash = sym;
		sym->hash = p;
	}

	if (sym->ns != NS_CPP)
		sym->id = newid();
	return linksym(sym);
}

Symbol *
newsym(int ns)
{
	Symbol *sym;

	sym = linksym(allocsym(ns, NULL));
	return sym;
}

Symbol *
newlabel(void)
{
	Symbol *sym = newsym(NS_LABEL);
	sym->id = newid();
	return sym;
}

Symbol *
lookup(int ns, char *name)
{
	Symbol *sym;
	int sns;
	char *t, c;

	c = *name;
	for (sym = htab[hash(name)]; sym; sym = sym->hash) {
		t = sym->name;
		if (*t != c || strcmp(t, name))
			continue;
		sns = sym->ns;
		if (sns == NS_KEYWORD || sns == NS_CPP || sns == ns)
			return sym;
	}
	return allocsym(ns, name);
}

void
delmacro(Symbol *sym)
{
	unlinkhash(sym);
	free(sym->name);
	free(sym->u.s);
	free(sym);
}

Symbol *
nextsym(Symbol *sym, int ns)
{
	char *s, *t, c;
	Symbol *p;

	/*
	 * This function is only called when a macro with parameters
	 * is called without them.
	 *      #define x(y) ((y) + 1)
	 *      int x = x(y);
	 */
	s = sym->name;
	c = *s;
	for (p = sym->hash; p; p = p->hash) {
		t = p->name;
		if (c == *t && !strcmp(s, t))
			return p;
	}
	return allocsym(ns, s);
}

Symbol *
install(int ns, Symbol *sym)
{
	if (sym->flags & ISDECLARED) {
		if (sym->ctx == curctx && ns == sym->ns)
			return NULL;
		sym = allocsym(ns, sym->name);
	}
	return linkhash(sym);
}

void
ikeywords(void)
{
	static struct {
		char *str;
		unsigned char token, value;
	} *bp, keywords[] = {
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
		{"inline", TQUALIFIER, INLINE},
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
	}, cppclauses[] = {
		{"define", DEFINE, DEFINE},
		{"include", INCLUDE, INCLUDE},
		{"line", LINE, LINE},
		{"ifdef", IFDEF, IFDEF},
		{"if", IF, IF},
		{"elif", ELIF, ELIF},
		{"else", ELSE, ELSE},
		{"ifndef", IFNDEF, IFNDEF},
		{"endif", ENDIF, ENDIF},
		{"undef", UNDEF, UNDEF},
		{"pragma", PRAGMA, PRAGMA},
		{"error", ERROR, ERROR},
		{NULL, 0, 0}
	}, *list[] = {
		keywords,
		cppclauses,
		NULL
	}, **lp;
	Symbol *sym;
	int ns = NS_KEYWORD;

	for (lp = list; *lp; ++lp) {
		for (bp = *lp; bp->str; ++bp) {
			sym = linkhash(allocsym(ns, bp->str));
			sym->token = bp->token;
			sym->u.token = bp->value;
		}
		ns = NS_CPPCLAUSES;
	}
	/*
	 * Remove all the predefined symbols from * the symbol list. It
	 * will make faster some operations. There is no problem of memory
	 * leakeage because this memory is not ever freed
	 */
	counterid = 0;
	head = NULL;
}
