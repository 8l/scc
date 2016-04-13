
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/cc.h"
#include "arch.h"
#include "cc1.h"

static void emitbin(unsigned, void *),
            emitswitcht(unsigned, void *),
            emitcast(unsigned, void *),
            emitswitch(unsigned, void *),
            emitsym(unsigned, void *),
            emitexp(unsigned, void *),
            emitsymid(unsigned, void *),
            emittext(unsigned, void *),
            emitfun(unsigned, void *),
            emitdcl(unsigned, void *),
            emitinit(unsigned, void *);

char *optxt[] = {
	[OADD] = "+",
	[OSUB] = "-",
	[OMUL] = "*",
	[OINC] = ":i",
	[ODEC] =  ":d",
	[OPTR] = "@",
	[OMOD] = "%",
	[ODIV] = "/",
	[OSHL] = "l",
	[OSHR]  = "r",
	[OLT] = "<",
	[OGT] = ">",
	[OGE] = "]",
	[OLE] =  "[",
	[OEQ] = "=",
	[ONE] = "!",
	[OBAND] = "&",
	[OBXOR]  = "^",
	[OBOR] = "|",
	[OASSIGN] = ":",
	[OA_MUL] = ":*",
	[OA_DIV] = ":/",
	[OA_MOD] = ":%",
	[OA_ADD] = ":+",
	[OA_SUB] = ":-",
	[OA_SHL] = ":l",
	[OA_SHR] = ":r",
	[OA_AND] = ":&",
	[OA_XOR] = ":^",
	[OA_OR] = ":|",
	[OADDR] = "'",
	[ONEG] = "_",
	[OCPL] = "~",
	[OAND] = "a",
	[OOR] = "o",
	[OASK] = "?",
	[OCOMMA] = ",",
	[OLABEL] = "L%d\n",
	[ODEFAULT] = "\tf\tL%d\n",
	[OCASE] = "\tv\tL%d",
	[OJUMP] = "\tj\tL%d\n",
	[OBRANCH] = "\ty\tL%d",
	[OEFUN] = "}\n",
	[OELOOP] = "\tb\n",
	[OBLOOP] = "\te\n",
	[ORET] = "\th",
	[OPAR] = "p",
	[OCALL] = "c",
	[OFIELD] = "."
};

void (*opcode[])(unsigned, void *) = {
	[OADD] = emitbin,
	[OSUB] = emitbin,
	[OMUL] = emitbin,
	[OINC] = emitbin,
	[ODEC] =  emitbin,
	[OPTR] = emitbin,
	[OMOD] = emitbin,
	[ODIV] = emitbin,
	[OSHL] = emitbin,
	[OSHR]  = emitbin,
	[OLT] = emitbin,
	[OGT] = emitbin,
	[OGE] = emitbin,
	[OLE] =  emitbin,
	[OEQ] = emitbin,
	[ONE] = emitbin,
	[OBAND] = emitbin,
	[OBXOR]  = emitbin,
	[OBOR] = emitbin,
	[OASSIGN] = emitbin,
	[OA_MUL] = emitbin,
	[OA_DIV] = emitbin,
	[OA_MOD] = emitbin,
	[OA_ADD] = emitbin,
	[OA_SUB] = emitbin,
	[OA_SHL] = emitbin,
	[OA_SHR] = emitbin,
	[OA_AND] = emitbin,
	[OA_XOR] = emitbin,
	[OA_OR] = emitbin,
	[OADDR] = emitbin,
	[ONEG] = emitbin,
	[OCPL] = emitbin,
	[OAND] = emitbin,
	[OOR] = emitbin,
	[OCOMMA] = emitbin,
	[OCAST] = emitcast,
	[OSYM] = emitsym,
	[OASK] = emitbin,
	[OCOLON] = emitbin,
	[OFIELD]= emitbin,
	[OEXPR] = emitexp,
	[OLABEL] = emitsymid,
	[ODEFAULT] = emitsymid,
	[OCASE] = emitsymid,
	[OJUMP] = emitsymid,
	[OBRANCH] = emitsymid,
	[OEFUN] = emittext,
	[OELOOP] = emittext,
	[OBLOOP] = emittext,
	[OFUN] = emitfun,
	[ORET] = emittext,
	[ODECL] = emitdcl,
	[OSWITCH] = emitswitch,
	[OSWITCHT] = emitswitcht,
	[OPAR] = emitbin,
	[OCALL] = emitbin,
	[OINIT] = emitinit
};

void
freetree(Node *np)
{
	if (!np)
		return;
	freetree(np->left);
	freetree(np->right);
	free(np);
}

static void
emitnode(Node *np)
{
	if (np)
		(*opcode[np->op])(np->op, np);
}

void
emit(unsigned op, void *arg)
{
	extern int failure;

	if (failure)
		return;
	(*opcode[op])(op, arg);
}

static void
emitvar(Symbol *sym)
{
	char c;
	short flags = sym->flags;

	if (flags & ISLOCAL)
		c = 'T';
	else if (flags & ISPRIVATE)
		c =  'Y';
	else if (flags & ISGLOBAL)
		c = 'G';
	else if (flags & ISREGISTER)
		c = 'R';
	else if (flags & ISFIELD)
		c = 'M';
	else if (flags & ISEXTERN)
		c = 'X';
	else
		c = 'A';
	printf("%c%u", c, sym->id);
}

static void
emitconst(Node *np)
{
	char *bp, c;
	Symbol *sym = np->sym;
	Type *tp = np->type;
	TUINT u;
	size_t n;

	switch (tp->op) {
	case PTR:
	case INT:
	case ENUM:
		u = (tp->sign) ? (TUINT) sym->u.i : sym->u.u;
		printf("#%c%llX",
		       np->type->letter,
		       (long long) sym->u.i & ones(tp->size));
		break;
	default:
		abort();
	}
}

static void
emitsym(unsigned op, void *arg)
{
	Node *np = arg;

	if ((np->sym->flags & ISINITLST) == 0) {
		/*
		 * When we have a compound literal we are going
		 * to call to emitnode for every element of it,
		 * and it means that we will have two '\t'
		 * for the first element
		 */
		putchar('\t');
	}
	(np->constant) ? emitconst(np) : emitvar(np->sym);
}

static void
emitletter(Type *tp)
{
	putchar(tp->letter);
	switch (tp->op) {
	case ARY:
	case STRUCT:
	case UNION:
		printf("%u", tp->id);
	}
}

static void
emittype(Type *tp)
{
	TINT n;
	Type **vp;
	Symbol **sp;
	char *tag;

	if (tp->printed || !tp->defined)
		return;
	tp->printed = 1;

	switch (tp->op) {
	case ARY:
		emittype(tp->type);
		emitletter(tp);
		putchar('\t');
		emitletter(tp->type);
		printf("\t#%c%d\n", sizettype->letter, tp->n.elem);
		return;
	case PTR:
		emittype(tp->type);
		return;
	case UNION:
	case STRUCT:
		n = tp->n.elem;
		for (sp = tp->p.fields; n-- > 0; ++sp)
			emittype((*sp)->type);
		emitletter(tp);
		tag = tp->tag->name;
		printf("\t\"%s\t#%c%llX\t#%c%llX\n",
		       (tag) ? tag : "",
		       sizettype->letter,
		       (unsigned long long) tp->size,
		       sizettype->letter,
		       (unsigned long long) tp->align);
		n = tp->n.elem;
		for (sp = tp->p.fields; n-- > 0; ++sp)
			emit(ODECL, *sp);
		break;
	case FTN:
		return;
	default:
		abort();
	}
}

static void
emitstring(Symbol *sym, Type *tp)
{
	char *bp, *s, *lim;
	int n;

	bp = bp = sym->u.s;
	lim = &sym->u.s[tp->n.elem];
	while (bp < lim) {
		s = bp;
		while (isprint(*bp) && bp < lim)
			++bp;
		if ((n = bp - s) > 1)
			printf("\t#\"%.*s\n", n, s);
		else
			bp = s;
		if (bp == lim)
			break;
		do {
			printf("\t#%c%02X\n",
			       chartype->letter, (*bp++) & 0xFF);
		} while (!isprint(*bp) && bp < lim);
	}
}

static void
emitdesig(Node *np, Type *tp)
{
	Symbol *sym;
	size_t n;
	Node *aux;
	Type *p;

	if (!np) {
		sym = NULL;
	} else {
		if (!np->sym)
			goto emit_expression;
		sym = np->sym;
		if (sym->flags & ISSTRING) {
			emitstring(sym, tp);
			return;
		}
		if ((sym->flags & ISINITLST) == 0)
			goto emit_expression;
	}

	switch (tp->op) {
	case PTR:
	case INT:
	case ENUM:
		aux = (sym) ? *sym->u.init : convert(constnode(zero), tp, 0);
		emitexp(OEXPR, aux);
		break;
	/* TODO: case UNION: */
	case STRUCT:
	case ARY:
		for (n = 0; n < tp->n.elem; ++n) {
			aux = (sym) ? sym->u.init[n] : NULL;
			p = (tp->op == ARY) ? tp->type : tp->p.fields[n]->type;
			emitdesig(aux, p);
		}
		break;
	default:
		abort();
	}

	if (sym) {
		free(sym->u.init);
		sym->u.init = NULL;
	}
	freetree(np);
	return;

emit_expression:
	emitexp(OEXPR, np);
}

static void
emitinit(unsigned op, void *arg)
{
	Node *np = arg;

	puts("\t(");
	emitdesig(np, np->type);
	puts(")");
}

static void
emitdcl(unsigned op, void *arg)
{
	Symbol *sym = arg;

	if (sym->flags & ISEMITTED)
		return;
	emittype(sym->type);
	emitvar(sym);
	putchar('\t');
	emitletter(sym->type);
	printf("\t\"%s", (sym->name) ? sym->name : "");
	if (sym->flags & ISFIELD)
		printf("\t#%c%llX", sizettype->letter, sym->u.i);
	sym->flags |= ISEMITTED;
	if ((sym->flags & HASINIT) == 0)
		putchar('\n');
}

static void
emitcast(unsigned op, void *arg)
{
	Node *np = arg, *lp = np->left;

	emitnode(lp);
	if (np->type != voidtype)
		printf("\tg%c", np->type->letter);
}

static void
emitbin(unsigned op, void *arg)
{
	Node *np = arg;
	char *s;

	emitnode(np->left);
	emitnode(np->right);
	if ((s = optxt[op]) != NULL)  {      /* do not print in OCOLON case */
		printf("\t%s", optxt[op]);
		emitletter(np->type);
	}
}

static void
emitexp(unsigned op, void *arg)
{
	Node *np = arg;

	emitnode(np);
	putchar('\n');
	freetree(np);
}

static void
emitfun(unsigned op, void *arg)
{
	Symbol *sym = arg, **sp;

	emitdcl(op, arg);
	puts("{");

	for (sp = sym->u.pars; sp && *sp; ++sp)
		emit(ODECL, *sp);
	puts("\\");
	free(sym->u.pars);
	sym->u.pars = NULL;
}

static void
emittext(unsigned op, void *arg)
{
	fputs(optxt[op], stdout);
}

static void
emitsymid(unsigned op, void *arg)
{
	Symbol *sym = arg;
	printf(optxt[op], sym->id);
}

static void
emitswitch(unsigned op, void *arg)
{
	Caselist *lcase = arg;

	printf("\ts\tL%u", lcase->ltable->id);
	emitexp(OEXPR, lcase->expr);
}

static void
emitswitcht(unsigned op, void *arg)
{
	Caselist *lcase = arg;
	struct scase *p, *next;

	printf("\tt\t#%c%0x\n", sizettype->letter, lcase->nr);
	for (p = lcase->head; p; p = next) {
		emitsymid(OCASE, p->label);
		emitexp(OEXPR, p->expr);
		next = p->next;
		free(p);
	}
	if (lcase->deflabel)
		emitsymid(ODEFAULT, lcase->deflabel);
}

Node *
node(unsigned op, Type *tp, Node *lp, Node *rp)
{
	Node *np;

	np = xmalloc(sizeof(*np));
	np->op = op;
	np->type = tp;
	np->sym = NULL;
	np->constant = np->lvalue = 0;
	np->left = lp;
	np->right = rp;

	return np;
}

Node *
varnode(Symbol *sym)
{
	Node *np;
	Type *tp = sym->type;

	np = node(OSYM, sym->type, NULL, NULL);
	np->type = sym->type;
	np->lvalue = tp->op != FTN && tp->op != ARY;
	np->constant = 0;
	np->sym = sym;
	return np;
}

Node *
constnode(Symbol *sym)
{
	Node *np;

	np = node(OSYM, sym->type, NULL, NULL);
	np->type = sym->type;
	np->constant = 1;
	np->sym = sym;
	return np;
}

Node *
sizeofnode(Type *tp)
{
	Symbol *sym;

	sym = newsym(NS_IDEN);
	sym->type = sizettype;
	sym->u.i = tp->size;
	return constnode(sym);
}
