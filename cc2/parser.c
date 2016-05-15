
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"

#include "arch.h"
#include "cc2.h"

#define MAXLINE     200
#define STACKSIZ     50

extern Type int8type, int16type, int32type, int64type,
            uint8type, uint16type, uint32type, uint64type,
            float32type, float64type, float80type,
            booltype,
            ptrtype,
            voidtype,
            elipsistype;

Type funtype = {
	.flags = FUNF
};

union tokenop {
	void *arg;
	unsigned op;
};

typedef void parsefun(char *, union tokenop);
static parsefun type, symbol, getname, unary, binary, ternary, call,
                constant, composed, binit, einit,
                jump, oreturn, loop, assign, casetbl;

typedef void evalfun(void);
static evalfun vardecl, beginfun, endfun, endpars, stmt,
               array, aggregate, flddecl, labeldcl;

static struct decoc {
	void (*eval)(void);
	void (*parse)(char *token, union tokenop);
	union tokenop u;
} optbl[] = {      /*  eval     parse           args */
	['A']   = {  vardecl,  symbol, .u.op  =  SAUTO<<8 | OAUTO},
	['R']   = {  vardecl,  symbol, .u.op  =   SREG<<8 |  OREG},
	['G']   = {  vardecl,  symbol, .u.op  =  SGLOB<<8 |  OMEM},
	['X']   = {  vardecl,  symbol, .u.op  = SEXTRN<<8 |  OMEM},
	['Y']   = {  vardecl,  symbol, .u.op  =  SPRIV<<8 |  OMEM},
	['T']   = {  vardecl,  symbol, .u.op  = SLOCAL<<8 |  OMEM},
	['M']   = {  flddecl,  symbol, .u.op  =  SMEMB<<8 |  OMEM},
	['L']   = { labeldcl,  symbol, .u.op  = SLABEL<<8 | OLABEL},

	['C']   = {     NULL,    type, .u.arg =    &int8type},
	['I']   = {     NULL,    type, .u.arg =   &int16type},
	['W']   = {     NULL,    type, .u.arg =   &int32type},
	['Q']   = {     NULL,    type, .u.arg =   &int64type},
	['K']   = {     NULL,    type, .u.arg =   &uint8type},
	['N']   = {     NULL,    type, .u.arg =  &uint16type},
	['Z']   = {     NULL,    type, .u.arg =  &uint32type},
	['O']   = {     NULL,    type, .u.arg =  &uint64type},
	['J']   = {     NULL,    type, .u.arg = &float32type},
	['D']   = {     NULL,    type, .u.arg = &float64type},
	['H']   = {     NULL,    type, .u.arg = &float80type},
	['0']   = {     NULL,    type, .u.arg =    &voidtype},
	['B']   = {     NULL,    type, .u.arg =    &booltype},
	['P']   = {     NULL,    type, .u.arg =     &ptrtype},
	['E']   = {     NULL,    type, .u.arg = &elipsistype},

	['F']   = {     NULL,    type, .u.arg =     &funtype},
	['V']   = {    array,composed,                     0},
	['U']   = {aggregate,composed,                     0},
	['S']   = {aggregate,composed,                     0},

	['"']   = {     NULL, getname,                     0},
	['{']   = { beginfun,    NULL,                     0},
	['}']   = {   endfun,    NULL,                     0},
	['(']   = {     NULL,   binit,                     0},
	[')']   = {     NULL,   einit,                     0},
	['\\']  = {  endpars,    NULL,                     0},
	['\t']  = {     stmt,    NULL,                     0},

	['~']   = {     NULL,   unary, .u.op =          OCPL},
	['-']   = {     NULL,   unary, .u.op =          ONEG},
	['\'']  = {     NULL,   unary, .u.op =         OADDR},
	['@']   = {     NULL,   unary, .u.op =          OPTR},
	['g']   = {     NULL,   unary, .u.op =         OCAST},
	['p']   = {     NULL,   unary, .u.op =          OPAR},

	['a']   = {     NULL,  binary, .u.op =          OAND},
	['o']   = {     NULL,  binary, .u.op =           OOR},
	['.']   = {     NULL,  binary, .u.op =        OFIELD},
	['+']   = {     NULL,  binary, .u.op =          OADD},
	['-']   = {     NULL,  binary, .u.op =          OSUB},
	['*']   = {     NULL,  binary, .u.op =          OMUL},
	['%']   = {     NULL,  binary, .u.op =          OMOD},
	['/']   = {     NULL,  binary, .u.op =          ODIV},
	['l']   = {     NULL,  binary, .u.op =          OSHL},
	['r']   = {     NULL,  binary, .u.op =          OSHR},
	['<']   = {     NULL,  binary, .u.op =           OLT},
	['>']   = {     NULL,  binary, .u.op =           OGT},
	['[']   = {     NULL,  binary, .u.op =           OLE},
	[']']   = {     NULL,  binary, .u.op =           OGE},
	['=']   = {     NULL,  binary, .u.op =           OEQ},
	['!']   = {     NULL,  binary, .u.op =           ONE},
	['&']   = {     NULL,  binary, .u.op =         OBAND},
	['|']   = {     NULL,  binary, .u.op =          OBOR},
	['^']   = {     NULL,  binary, .u.op =         OBXOR},
	[',']   = {     NULL,  binary, .u.op =        OCOMMA},

	[':']   = {     NULL,  assign, .u.op =        OASSIG},
	['?']   = {     NULL, ternary, .u.op =          OASK},
	['c']   = {     NULL,    call, .u.op =         OCALL},

	['#']   = {     NULL,constant, .u.op =        OCONST},

	['j']   = {     NULL,    jump, .u.op =          OJMP},
	['y']   = {     NULL,    jump, .u.op =       OBRANCH},
	['h']   = {     NULL, oreturn, .u.op =          ORET},

	['b']   = {     NULL,    loop, .u.op =        OBLOOP},
	['e']   = {     NULL,    loop, .u.op =        OELOOP},

	['v']   = {     NULL,    jump, .u.op =         OCASE},
	['s']   = {     NULL,    jump, .u.op =       OSWITCH},

	['f']   = {     NULL, casetbl, .u.op =      ODEFAULT},
	['t']   = {     NULL, casetbl, .u.op =        OTABLE}
};

static int sclass, inpars, ininit, endf, lineno;
static void *stack[STACKSIZ], **sp = stack;

static void
push(void *elem)
{
	if (sp == stack[STACKSIZ])
		error(ESTACKO);
	*sp++ = elem;
}

static void *
pop(void)
{
	if (sp == stack)
		error(ESTACKU);
	return *--sp;
}

static int
empty(void)
{
	return sp == stack;
}

static void
type(char *token, union tokenop u)
{
	push(u.arg);
}

static void
composed(char *token, union tokenop u)
{
	Symbol *sym;

	sym = getsym(atoi(token+1));
	push(&sym->type);
}

static void
getname(char *t, union tokenop u)
{
	push((*++t) ? xstrdup(t) : NULL);
}

static void
symbol(char *token, union tokenop u)
{
	Node *np = newnode(u.op & 0xFF);
	Symbol *sym = getsym(atoi(token+1));

	sclass = u.op >> 8;
	np->u.sym = sym;
	np->type = sym->type;
	push(np);
}

static Type *
gettype(char *token)
{
	struct decoc *dp;

	dp = &optbl[*token];
	if (!dp->parse)
		error(ESYNTAX);
	(*dp->parse)(token, dp->u);
	return pop();
}

static void
constant(char *token, union tokenop u)
{
	static char letters[] = "0123456789ABCDEF";
	Node *np;
	TUINT v;
	unsigned c;

	++token;
	if (*token == OSTRING) {
		++token;
		np = newnode(OSTRING);
		np->type.flags = STRF;
		np->type.size = strlen(token);
		np->type.align = int8type.align;
		np->u.s = xstrdup(token);
	} else {
		np = newnode(OCONST);
		np->type = *gettype(token++);
		for (v = 0; c = *token++; v += c) {
			v <<= 4;
			c = strchr(letters, c) - letters;
		}
		np->u.i = v;
	}
	push(np);
}

static void
assign(char *token, union tokenop u)
{
	int subop;
	Node *np = newnode(u.op);

	switch (subop = *++token) {
	case '/':
	case '%':
	case '+':
	case '-':
	case 'l':
	case 'r':
	case '&':
	case '|':
	case '^':
	case 'i':
	case 'd':
		++token;
		subop = optbl[subop].u.op;
		break;
	default:
		subop = 0;
		break;
	}

	np->u.subop = subop;
	np->type = *gettype(token);
	np->right = pop();
	np->left = pop();
	push(np);
}

static void
ternary(char *token, union tokenop u)
{
	Node *ask = newnode(OCOLON), *colon = newnode(OASK);
	Type *tp = gettype(token+1);

	colon->right = pop();
	colon->left = pop();

	ask->type = *tp;
	ask->left = pop();
	push(ask);
}

static Node *
eval(char *tok)
{
	struct decoc *dp;

	do {
		dp = &optbl[*tok];
		if (!dp->parse)
			break;
		(*dp->parse)(tok, dp->u);
	} while (tok = strtok(NULL, "\t\n"));
}

static int
nextline(void)
{
	char line[MAXLINE];
	size_t len;
	int c;
	void (*fun)(void);

repeat:
	++lineno;
	if (!fgets(line, sizeof(line), stdin))
		return 0;
	if ((len = strlen(line)) == 0 || line[0] == '\n')
		goto repeat;
	if (line[len-1] != '\n')
		error(len < sizeof(line)-1 ? ELNBLNE : ELNLINE);
	line[len-1] = '\0';

	c = *line;
	eval(strtok(line, "\t\n"));
	if ((fun = *optbl[c].eval) != NULL)
		(*fun)();
	if (sp != stack)
		error(ESTACKA);
	return 1;
}

static void
oreturn(char *token, union tokenop u)
{
	Node *np = newnode(u.op);

	eval(strtok(NULL, "\t\n"));
	if (!empty())
		np->left = pop();
	push(np);
}

static void
jump(char *token, union tokenop u)
{
	Node *aux, *np = newnode(u.op);

	eval(strtok(NULL, "\t\n"));

	if (u.op != OJMP)
		np->left = pop();
	aux = pop();
	np->u.sym = aux->u.sym;
	delnode(aux);
	push(np);
}

static void
casetbl(char *token, union tokenop u)
{
	Node *np = newnode(u.op);

	eval(strtok(NULL, "\t\n"));
	np->left = pop();
	push(np);
}

static void
loop(char *token, union tokenop u)
{
	push(newnode(u.op));
}

static void
unary(char *token, union tokenop u)
{
	Node *np = newnode(u.op);

	np->type = *gettype(token+1);
	np->left = pop();
	np->right = NULL;
	push(np);
}

static void
call(char *token, union tokenop u)
{
	Node *np, *par, *fun = newnode(u.op);

	for (par = NULL;; par = np) {
		np = pop();
		if (np->op != OPAR)
			break;
		np->right = par;
	}

	fun->type = *gettype(token+1);
	fun->left = np;
	fun->right = par;
	push(fun);
}

static void
binary(char *token, union tokenop u)
{
	Node *np = newnode(u.op);

	np->type = *gettype(token+1);
	np->right = pop();
	np->left = pop();
	push(np);
}

static void
binit(char *token, union tokenop u)
{
	ininit = 1;
}

static void
einit(char *token, union tokenop u)
{
	ininit = 0;
	endinit();
}

static void
endpars(void)
{
	if (!curfun || !inpars)
		error(ESYNTAX);
	inpars = 0;
}

static void
aggregate(void)
{
	Node *align, *size;
	char *name;
	Type *tp;
	Symbol *sym;

	align = pop();
	size = pop();
	name = pop();
	tp = pop();

	tp->size = size->u.i;
	tp->align = align->u.i;
	/*
	 * type is the first field of Symbol so we can obtain the
	 * address of the symbol from the address of the type.
	 * We have to do this because composed returns the pointer
	 * to the type, but in this function we also need the
	 * symbol to store the name.
	 */
	sym = (Symbol *) tp;
	sym->name = name;

	delnode(align);
	delnode(size);
}

static void
array(void)
{
	Type *tp, *base;
	Node *size;

	size = pop();
	base = pop();
	tp = pop();
	tp->size = size->u.i;
	tp->align = base->align;

	delnode(size);
}

static void
decl(Symbol *sym)
{
	Type *tp = &sym->type;

	if (tp->flags & FUNF) {
		curfun = sym;
	} else {
		switch (sym->kind) {
		case SEXTRN:
		case SGLOB:
		case SPRIV:
		case SLOCAL:
			defglobal(sym);
			break;
		case SAUTO:
		case SREG:
			if (!curfun)
				error(ESYNTAX);
			((inpars) ? defpar : defvar)(sym);
			break;
		default:
			abort();
		}
	}
}

static void
vardecl(void)
{
	Type *tp, *rp;
	Node *np;
	Symbol *sym;
	char *name;

	name = pop();
	tp = pop();
	if (tp->flags & FUNF)
		rp = pop();
	np = pop();

	sym = np->u.sym;
	/*
	 * We have to free sym->name because in tentative declarations
	 * we can have multiple declarations of the same symbol, and in
	 * this case our parser will allocate twice the memory
	 */
	free(sym->name);
	sym->name = name;
	sym->type = *tp;
	if (tp->flags & FUNF)
		rtype = *rp;
	sym->kind = sclass;

	if (ininit)
		sym->type.flags |= INITF;
	decl(sym);
	delnode(np);
}

static void
flddecl(void)
{
	Node *off, *np;
	char *name;
	Type *tp;
	Symbol *sym;

	off = pop();
	name = pop();
	tp = pop();
	np = pop();

	sym = np->u.sym;
	sym->u.off = off->u.i;
	sym->name = name;
	sym->type = *tp;

	delnode(np);
	delnode(off);
}

static void
labeldcl(void)
{
	Node *np;
	Symbol *sym;

	np = pop();
	np->op = ONOP;
	sym = np->u.sym;
	sym->kind = SLABEL;
	sym->u.stmt = np;
	np->label = sym;
	addstmt(np, SETCUR);
}

static void
stmt(void)
{
	Node *np;

	np = pop();
	if (ininit) {
		data(np);
		deltree(np);
		return;
	}
	addstmt(np, SETCUR);
}

static void
beginfun(void)
{
	inpars = 1;
	pushctx();
}

static void
endfun(void)
{
	endf = 1;
}

void
parse(void)
{
	cleannodes();  /* remove code of previous function */
	popctx();  /* remove context of previous function */
	curfun = NULL;
	endf = 0;

	while (!endf && nextline())
		/* nothing */;
	if (ferror(stdin))
		error(EFERROR, strerror(errno));
}
