
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"
#include "arch.h"
#include "cc1.h"


typedef struct init Init;

struct designator {
	TINT pos;
	Node *expr;
	struct designator *next;
};

struct init {
	Type *type;
	size_t pos;
	size_t max;
	struct designator *tail;
	struct designator *head;
};


static TINT
arydesig(Init *ip)
{
	TINT npos;
	Node *np;
	Type *tp = ip->type;

	if (ip->type->op != ARY)
		errorp("array index in non-array initializer");
	next();
	np = iconstexpr();
	npos = np->sym->u.i;
	if (npos < 0 || tp->defined && npos >= tp->n.elem) {
		errorp("array index in initializer exceeds array bounds");
		npos = 0;
	}
	freetree(np);
	expect(']');
	return npos;
}

static TINT
fielddesig(Init *ip)
{
	int ons;
	Symbol *sym, **p;
	Type *tp = ip->type;

	if (!tp->aggreg)
		errorp("field name not in record or union initializer");
	ons = namespace;
	namespace = tp->ns;
	next();
	namespace = ons;
	if (yytoken != IDEN)
		unexpected();
	sym = yylval.sym;
	if ((sym->flags & SDECLARED) == 0) {
		errorp(" unknown field '%s' specified in initializer",
		      sym->name);
		return 0;
	}
	for (p = tp->p.fields; *p != sym; ++p)
		/* nothing */;
	next();
	return p - tp->p.fields;
}

static Init *
designation(Init *ip)
{
	TINT (*fun)(Init *);

	switch (yytoken) {
	case '[': fun = arydesig;   break;
	case '.': fun = fielddesig; break;
	default:  return ip;
	}

	ip->pos  = (*fun)(ip);
	expect('=');
	return ip;
}

static Node *
initialize(Type *tp)
{
	Node *np, *aux;
	Symbol *sym;
	Type *btp;
	size_t len;
	char *s;
	int isstring;

	if ((tp->op == ARY || tp->op == STRUCT) &&
	    yytoken != '{' && yytoken != STRING) {
		return initlist(tp);
	}

	isstring = yytoken == STRING;
	np = (yytoken == '{') ? initlist(tp) : assign();

	if (isstring && tp->op == ARY) {
		sym = np->left->sym;
		btp = tp->type;
		if (btp != chartype  &&
		    btp != uchartype &&
		    btp != schartype) {
			errorp("array of inappropriate type initialized from string constant");
			goto return_zero;
		}
		len = sym->type->n.elem-1;
		if (!tp->defined) {
			tp->defined = 1;
			tp->n.elem = len+1;
		} else if (tp->n.elem < len) {
			warn("initializer-string for array of chars is too long");
		}
		len = tp->n.elem;
		s = sym->u.s;
		sym = newstring(NULL, len);
		strncpy(sym->u.s, s, len);
		np->sym = sym;
		np->type = sym->type;

		return np;
	}
	if (eqtype(tp, np->type))
		return np;
	if ((aux = convert(decay(np), tp, 0)) != NULL)
		return aux;
	errorp("incorrect initializer");

return_zero:
	return constnode(zero);
}

static Node *
mkcompound(Init *ip)
{
	Node **v, **p;
	size_t n;
	struct designator *dp, *next;
	Symbol *sym;

	if ((n = ip->max) == 0) {
		v = NULL;
	} else if (n > SIZE_MAX / sizeof(*v)) {
		errorp("compound literal too big");
		return constnode(zero);
	} else {
		n *= sizeof(*v);
		v = memset(xmalloc(n), 0, n);

		for (dp = ip->head; dp; dp = next) {
			p = &v[dp->pos];
			freetree(*p);
			*p = dp->expr;
			next = dp->next;
			free(dp);
		}
	}

	sym = newsym(NS_IDEN);
	sym->u.init = v;
	sym->type = ip->type;
	sym->flags |= SINITLST;

	return constnode(sym);
}

static void
newdesig(Init *ip, Node *np)
{
	struct designator *dp;

	dp = xmalloc(sizeof(*dp));
	dp->pos = ip->pos;
	dp->expr = np;
	dp->next = NULL;

	if (ip->head == NULL) {
		ip->head = ip->tail = dp;
	} else {
		ip->tail->next = dp;
		ip->tail = dp;
	}
}

Node *
initlist(Type *tp)
{
	Init in;
	int braces, scalar, toomany, outbound;
	Type *newtp;
	Node *np;

	in.tail = in.head = NULL;
	in.type = tp;
	in.pos = 0;
	in.max = 0;
	braces = scalar = toomany = 0;

	if (accept('{'))
		braces = 1;

	do {
		if (yytoken == '}')
			break;
		outbound = 0;
		designation(&in);
		switch (tp->op) {
		case ARY:
			newtp = tp->type;
			if (!tp->defined || in.pos < tp->n.elem)
				break;
			if (!toomany)
				warn("excess elements in array initializer");
			outbound = 1;
			toomany = 1;
			break;
		/* TODO: case UNION: */
		case STRUCT:
			if (in.pos < tp->n.elem) {
				newtp = tp->p.fields[in.pos]->type;
				break;
			}
			newtp = inttype;
			if (!toomany)
				warn("excess elements in struct initializer");
			toomany = 1;
			outbound = 1;
			break;
		default:
			newtp = tp;
			if (!scalar)
				warn("braces around scalar initializer");
			scalar = 1;
			if (in.pos == 0)
				break;
			if (!toomany)
				warn("excess elements in scalar initializer");
			toomany = 1;
			outbound = 1;
			break;
		}

		np = initialize(newtp);
		if (outbound)
			freetree(np);
		else
			newdesig(&in, np);

		if (++in.pos == 0)
			errorp("compound literal too big");
		if (in.pos > in.max)
			in.max = in.pos;
		if (tp->n.elem == in.pos && !braces)
			break;
	} while (accept(','));

	if (braces)
		expect('}');

	if (tp->op == ARY && !tp->defined) {
		tp->n.elem = in.max;
		tp->defined = 1;
	}
	if (tp->op == ARY || tp->op == STRUCT)
		in.max = tp->n.elem;
	else if (in.max == 0) {
		errorp("empty scalar initializer");
		return constnode(zero);
	}

	return mkcompound(&in);
}

void
initializer(Symbol *sym, Type *tp)
{
	Node *np;
	int flags = sym->flags;

	if (tp->op == FTN) {
		errorp("function '%s' is initialized like a variable",
		       sym->name);
		tp = inttype;
	}
	np = initialize(tp);

	if (flags & SDEFINED) {
		errorp("redeclaration of '%s'", sym->name);
	} else if ((flags & (SGLOBAL|SLOCAL|SPRIVATE)) != 0) {
		if (!(np->flags & NCONST)) {
			errorp("initializer element is not constant");
			return;
		}
		sym->flags |= SHASINIT;
		sym->flags &= ~SEMITTED;
		emit(ODECL, sym);
		emit(OINIT, np);
		sym->flags |= SDEFINED;
	} else if ((flags & (SEXTERN|STYPEDEF)) != 0) {
		errorp("'%s' has both '%s' and initializer",
		       sym->name, (flags&SEXTERN) ? "extern" : "typedef");
	} else {
		emit(ODECL, sym);
		np = node(OASSIGN, tp, varnode(sym), np);
		emit(OEXPR, np);
	}
}
