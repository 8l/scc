
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/cc.h"
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
	[OBRANCH] = "\tj\tL%d",
	[OEFUN] = "}\n",
	[OELOOP] = "\tb\n",
	[OBLOOP] = "\te\n",
	[ORET] = "\tr",
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
		c = L_LOCAL;
	else if (flags & ISPRIVATE)
		c =  L_PRIVATE;
	else if (flags & ISGLOBAL)
		c = L_PUBLIC;
	else if (flags & ISREGISTER)
		c = L_REGISTER;
	else if (flags & ISFIELD)
		c = L_FIELD;
	else if (flags & ISEXTERN)
		c = L_EXTERN;
	else
		c = L_AUTO;
	printf("%c%u", c, sym->id);
}

static void
emitconst(Node *np)
{
	char *bp, c;
	Symbol *sym = np->sym;
	Type *tp = np->type;
	TUINT u;

	switch (tp->op) {
	case PTR:
	case INT:
		u = (tp->sign) ? (TUINT) sym->u.i : sym->u.u;
		printf("#%c%lX", np->type->letter, sym->u.i & ones(tp->size));
		break;
	case ARY:
		/*
		 * FIX: At this point we are going to assume
		 * that all the arrays are strings
		 */
		putchar('"');
		for (bp = sym->u.s; c = *bp; ++bp)
			printf("%02X", c & 0xFF);
		break;
	default:
		/* TODO: Handle other kind of constants */
		abort();
	}
}

static void
emitsym(unsigned op, void *arg)
{
	Node *np = arg;
	putchar('\t');
	(np->constant) ? emitconst(np) : emitvar(np->sym);
}

static void
emitletter(Type *tp)
{
	putchar(tp->letter);
	switch (tp->op) {
	case ARY:
	case FTN:
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
		printf("\t#%d\n", tp->n.elem);
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
		if ((tag = tp->tag->name) != NULL)
			printf("\t%s", tag);
		putchar('\n');
		n = tp->n.elem;
		for (sp = tp->p.fields; n-- > 0; ++sp)
			emit(ODECL, *sp);
		break;
	case FTN:
		emitletter(tp);
		putchar('\t');
		emitletter(tp->type);
		n = tp->n.elem;
		for (vp = tp->p.pars; n-- > 0; ++vp) {
			putchar('\t');
			emitletter(*vp);
		}
		putchar('\n');
		return;
	default:
		abort();
	}
}

static void
emitinit(unsigned op, void *arg)
{
	Node *np = arg;

	puts("(");
	emitexp(OEXPR, np->right);
	puts(")");
	np->right = NULL;
	freetree(np);
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
	if (sym->name)
		printf("\t%s", sym->name);
	if (op != OFUN)
		putchar('\n');
	sym->flags |= ISEMITTED;
}

static void
emitcast(unsigned op, void *arg)
{
	Node *np = arg, *lp = np->left;

	emitnode(lp);
	if (np->type != voidtype)
		printf("\tg%c", lp->type->letter, np->type->letter);
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
	TINT n;

	emitdcl(op, arg);
	puts("\n{");

	n = sym->type->n.elem;
	for (sp = sym->u.pars; n-- > 0; ++sp) {
		if ((sym = *sp) == NULL)
			continue;
		/* enable non used warnings in parameters */
		sym->flags &= ~ISUSED;
		emit(ODECL, sym);
	}
	puts("\\");
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

	printf("\tt\t#%0x\n", lcase->nr);
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
	np->constant = np->symbol = np->lvalue = 0;
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
	np->symbol = 1;
	np->sym = sym;
	return np;
}

Node *
constnode(Symbol *sym)
{
	Node *np;

	np = node(OSYM, sym->type, NULL, NULL);
	np->type = sym->type;
	np->symbol = 1;
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
