/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/arch/qbe/code.c";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstd.h>
#include "arch.h"
#include "../../../inc/cc.h"
#include "../../cc2.h"

#define ADDR_LEN (INTIDENTSIZ+64)

static void binary(void), unary(void), store(void), jmp(void), ret(void),
            branch(void), call(void), ecall(void), param(void),
            alloc(void), form2local(void), ldir(void), vastart(void),
            vaarg(void);

static struct opdata {
	void (*fun)(void);
	char *txt;
	char letter;
} optbl [] = {
	[ASLDSB]  =  {.fun = unary,  .txt = "loadsb", .letter = 'w'},
	[ASLDUB]  =  {.fun = unary,  .txt = "loadub", .letter = 'w'},
	[ASLDSH]  =  {.fun = unary,  .txt = "loadsh", .letter = 'w'},
	[ASLDUH]  =  {.fun = unary,  .txt = "loaduh", .letter = 'w'},
	[ASLDSW]  =  {.fun = unary,  .txt = "loadsw", .letter = 'w'},
	[ASLDUW]  =  {.fun = unary,  .txt = "loaduw", .letter = 'w'},
	[ASLDL]   =  {.fun = unary,  .txt = "loadl", .letter = 'l'},
	[ASLDS]   =  {.fun = unary,  .txt = "loads", .letter = 's'},
	[ASLDD]   =  {.fun = unary,  .txt = "loadd", .letter = 'd'},

	[ASCOPYB] =  {.fun = unary,  .txt = "copy", .letter = 'b'},
	[ASCOPYH] =  {.fun = unary,  .txt = "copy", .letter = 'h'},
	[ASCOPYW] =  {.fun = unary,  .txt = "copy", .letter = 'w'},
	[ASCOPYL] =  {.fun = unary,  .txt = "copy", .letter = 'l'},
	[ASCOPYS] =  {.fun = unary,  .txt = "copy", .letter = 's'},
	[ASCOPYD] =  {.fun = unary,  .txt = "copy", .letter = 'd'},

	[ASSTB]   =  {.fun = store,  .txt = "store", .letter = 'b'},
	[ASSTH]   =  {.fun = store,  .txt = "store", .letter = 'h'},
	[ASSTW]   =  {.fun = store,  .txt = "store", .letter = 'w'},
	[ASSTL]   =  {.fun = store,  .txt = "store", .letter = 'l'},
	[ASSTM]   =  {.fun = ldir},
	[ASSTS]   =  {.fun = store,  .txt = "store", .letter = 's'},
	[ASSTD]   =  {.fun = store,  .txt = "store", .letter = 'd'},

	[ASADDW]  =  {.fun = binary, .txt = "add", .letter = 'w'},
	[ASSUBW]  =  {.fun = binary, .txt = "sub", .letter = 'w'},
	[ASMULW]  =  {.fun = binary, .txt = "mul", .letter = 'w'},
	[ASMODW]  =  {.fun = binary, .txt = "rem", .letter = 'w'},
	[ASUMODW] =  {.fun = binary, .txt = "urem", .letter = 'w'},
	[ASDIVW]  =  {.fun = binary, .txt = "div", .letter = 'w'},
	[ASUDIVW] =  {.fun = binary, .txt = "udiv", .letter = 'w'},
	[ASSHLW]  =  {.fun = binary, .txt = "shl", .letter = 'w'},
	[ASSHRW]  =  {.fun = binary, .txt = "sar", .letter = 'w'},
	[ASUSHRW] =  {.fun = binary, .txt = "shr", .letter = 'w'},
	[ASLTW]   =  {.fun = binary, .txt = "csltw", .letter = 'w'},
	[ASULTW]  =  {.fun = binary, .txt = "cultw", .letter = 'w'},
	[ASGTW]   =  {.fun = binary, .txt = "csgtw", .letter = 'w'},
	[ASUGTW]  =  {.fun = binary, .txt = "cugtw", .letter = 'w'},
	[ASLEW]   =  {.fun = binary, .txt = "cslew", .letter = 'w'},
	[ASULEW]  =  {.fun = binary, .txt = "culew", .letter = 'w'},
	[ASGEW]   =  {.fun = binary, .txt = "csgew", .letter = 'w'},
	[ASUGEW]  =  {.fun = binary, .txt = "cugew", .letter = 'w'},
	[ASEQW]   =  {.fun = binary, .txt = "ceqw", .letter = 'w'},
	[ASNEW]   =  {.fun = binary, .txt = "cnew", .letter = 'w'},
	[ASBANDW] =  {.fun = binary, .txt = "and", .letter = 'w'},
	[ASBORW]  =  {.fun = binary, .txt = "or", .letter = 'w'},
	[ASBXORW] =  {.fun = binary, .txt = "xor", .letter = 'w'},

	[ASADDL]  =  {.fun = binary, .txt = "add", .letter = 'l'},
	[ASSUBL]  =  {.fun = binary, .txt = "sub", .letter = 'l'},
	[ASMULL]  =  {.fun = binary, .txt = "mul", .letter = 'l'},
	[ASMODL]  =  {.fun = binary, .txt = "rem", .letter = 'l'},
	[ASUMODL] =  {.fun = binary, .txt = "urem", .letter = 'l'},
	[ASDIVL]  =  {.fun = binary, .txt = "div", .letter = 'l'},
	[ASUDIVL] =  {.fun = binary, .txt = "udiv", .letter = 'l'},
	[ASSHLL]  =  {.fun = binary, .txt = "shl", .letter = 'l'},
	[ASSHRL]  =  {.fun = binary, .txt = "sar", .letter = 'l'},
	[ASUSHRL] =  {.fun = binary, .txt = "shr", .letter = 'l'},
	[ASLTL]   =  {.fun = binary, .txt = "csltl", .letter = 'w'},
	[ASULTL]  =  {.fun = binary, .txt = "cultl", .letter = 'w'},
	[ASGTL]   =  {.fun = binary, .txt = "csgtl", .letter = 'w'},
	[ASUGTL]  =  {.fun = binary, .txt = "cugtl", .letter = 'w'},
	[ASLEL]   =  {.fun = binary, .txt = "cslel", .letter = 'w'},
	[ASULEL]  =  {.fun = binary, .txt = "culel", .letter = 'w'},
	[ASGEL]   =  {.fun = binary, .txt = "csgel", .letter = 'w'},
	[ASUGEL]  =  {.fun = binary, .txt = "cugel", .letter = 'w'},
	[ASEQL]   =  {.fun = binary, .txt = "ceql", .letter = 'w'},
	[ASNEL]   =  {.fun = binary, .txt = "cnel", .letter = 'w'},
	[ASBANDL] =  {.fun = binary, .txt = "and", .letter = 'l'},
	[ASBORL]  =  {.fun = binary, .txt = "or", .letter = 'l'},
	[ASBXORL] =  {.fun = binary, .txt = "xor", .letter = 'l'},

	[ASADDS]  =  {.fun = binary, .txt = "add", .letter = 's'},
	[ASSUBS]  =  {.fun = binary, .txt = "sub", .letter = 's'},
	[ASMULS]  =  {.fun = binary, .txt = "mul", .letter = 's'},
	[ASDIVS]  =  {.fun = binary, .txt = "div", .letter = 's'},
	[ASLTS]   =  {.fun = binary, .txt = "clts", .letter = 'w'},
	[ASGTS]   =  {.fun = binary, .txt = "cgts", .letter = 'w'},
	[ASLES]   =  {.fun = binary, .txt = "cles", .letter = 'w'},
	[ASGES]   =  {.fun = binary, .txt = "cges", .letter = 'w'},
	[ASEQS]   =  {.fun = binary, .txt = "ceqs", .letter = 'w'},
	[ASNES]   =  {.fun = binary, .txt = "cnes", .letter = 'w'},

	[ASADDD]  =  {.fun = binary, .txt = "add", .letter = 'd'},
	[ASSUBD]  =  {.fun = binary, .txt = "sub", .letter = 'd'},
	[ASMULD]  =  {.fun = binary, .txt = "mul", .letter = 'd'},
	[ASDIVD]  =  {.fun = binary, .txt = "div", .letter = 'd'},
	[ASLTD]   =  {.fun = binary, .txt = "cltd", .letter = 'w'},
	[ASGTD]   =  {.fun = binary, .txt = "cgtd", .letter = 'w'},
	[ASLED]   =  {.fun = binary, .txt = "cled", .letter = 'w'},
	[ASGED]   =  {.fun = binary, .txt = "cged", .letter = 'w'},
	[ASEQD]   =  {.fun = binary, .txt = "ceqd", .letter = 'w'},
	[ASNED]   =  {.fun = binary, .txt = "cned", .letter = 'w'},

	[ASEXTBW] =  {.fun = unary, .txt = "extsb", .letter = 'w'},
	[ASUEXTBW]=  {.fun = unary, .txt = "extub", .letter = 'w'},
	[ASEXTBL] =  {.fun = unary, .txt = "extsb", .letter = 'l'},
	[ASUEXTBL]=  {.fun = unary, .txt = "extub", .letter = 'l'},
	[ASEXTHW] =  {.fun = unary, .txt = "extsh", .letter = 'w'},
	[ASUEXTHW]=  {.fun = unary, .txt = "extuh", .letter = 'w'},
	[ASEXTWL] =  {.fun = unary, .txt = "extsw", .letter = 'l'},
	[ASUEXTWL]=  {.fun = unary, .txt = "extuw", .letter = 'l'},

	[ASSTOL] = {.fun = unary, .txt = "stosi", .letter = 'l'},
	[ASSTOW] = {.fun = unary, .txt = "stosi", .letter = 'w'},
	[ASDTOL] = {.fun = unary, .txt = "dtosi", .letter = 'l'},
	[ASDTOW] = {.fun = unary, .txt = "dtosi", .letter = 'w'},

	[ASSWTOD] = {.fun = unary, .txt = "swtof", .letter = 'd'},
	[ASSWTOS] = {.fun = unary, .txt = "swtof", .letter = 's'},
	[ASSLTOD] = {.fun = unary, .txt = "sltof", .letter = 'd'},
	[ASSLTOS] = {.fun = unary, .txt = "sltof", .letter = 's'},

	[ASEXTS] = {.fun = unary, .txt = "exts", .letter = 'd'},
	[ASTRUNCD] = {.fun = unary, .txt = "truncd", .letter = 's'},

	[ASBRANCH] = {.fun = branch},
	[ASJMP]  = {.fun = jmp},
	[ASRET]  = {.fun = ret},
	[ASCALL] = {.fun = call},
	[ASCALLE] = {.fun = ecall, .txt = ")"},
	[ASCALLEX] = {.fun = ecall, .txt = ", ...)"},
	[ASPAR] = {.fun = param, .txt = "%s %s, "},
	[ASPARE] = {.fun = param, .txt = "%s %s"},
	[ASALLOC] = {.fun = alloc},
	[ASFORM] = {.fun = form2local},

	[ASVSTAR] = {.fun = vastart},
	[ASVARG] = {.fun = vaarg},
};

static char buff[ADDR_LEN];
/*
 * : is for user-defined Aggregate Types
 * $ is for globals (represented by a pointer)
 * % is for function-scope temporaries
 * @ is for block labels
 */
static char
sigil(Symbol *sym)
{
	switch (sym->kind) {
	case SEXTRN:
	case SGLOB:
	case SPRIV:
	case SLOCAL:
		return '$';
	case SAUTO:
	case STMP:
		return '%';
	case SLABEL:
		return '@';
	default:
		abort();
	}
}

static char *
symname(Symbol *sym)
{
	char c = sigil(sym);

	if (sym->name) {
		switch (sym->kind) {
		case SEXTRN:
		case SGLOB:
			sprintf(buff, "%c%s", c, sym->name);
			return buff;
		case SLOCAL:
		case SPRIV:
		case SAUTO:
			sprintf(buff, "%c%s.%u", c, sym->name, sym->id);
			return buff;
		default:
			abort();
		}
	}
	sprintf(buff, "%c.%u", c, sym->numid);

	return buff;
}

static void
emitconst(Node *np)
{
	switch (np->type.size) {
	case 1:
		printf("%d", (int) np->u.i & 0xFF);
		break;
	case 2:
		printf("%d", (int) np->u.i & 0xFFFF);
		break;
	case 4:
		printf("%ld", (long) np->u.i & 0xFFFFFFFF);
		break;
        case 8:
                printf("%lld", (long long) np->u.i & 0xFFFFFFFF);
                break;
	default:
		abort();
	}
}

static void
emittree(Node *np)
{
	if (!np)
		return;

	switch (np->op) {
	case OSTRING:
		printf("\"%s\"", np->u.s);
		free(np->u.s);
		np->u.s = NULL;
		break;
	case OCONST:
		emitconst(np);
		break;
	case OADDR:
		emittree(np->left);
		break;
	case OMEM:
		fputs(symname(np->u.sym), stdout);
		break;
	default:
		emittree(np->left);
		printf(" %c ", np->op);
		emittree(np->right);
		break;
	}
}

static char *
size2asm(Type *tp)
{
	if (tp->flags & STRF) {
		return "b";
	} else if (tp->flags & INTF) {
		switch (tp->size) {
		case 1:
			return "b";
		case 2:
			return "h";
		case 4:
			return "w";
		case 8:
			return "l";
		}
	} else if (tp->flags & FLOATF) {
		if (tp->size == 4)
			return "s";
		else if (tp->size == 8)
			return "d";
	}
	abort();
}

void
defglobal(Symbol *sym)
{
	if (sym->kind == SEXTRN)
		return;
	if (sym->kind == SGLOB)
		fputs("export ", stdout);
	printf("data %s = {\n", symname(sym));
	if (sym->type.flags & INITF)
		return;
	printf("\tz\t%lu\n}\n", sym->type.size);
}

void
defpar(Symbol *sym)
{
	sym->type.flags |= PARF;
}

void
defvar(Symbol *sym)
{
	if (sym->kind == SREG)
		sym->kind = SAUTO;
}

void
data(Node *np)
{
	printf("\t%s\t", size2asm(&np->type));
	emittree(np);
	putchar(',');
	putchar('\n');
}

static char *
size2stack(Type *tp)
{
	if (tp->flags & INTF) {
		switch (tp->size) {
		case 1:
		case 2:
		case 4:
			return "w";
		case 8:
			return "l";
		}
	} else if (tp->flags & FLOATF) {
		if (tp->size == 4)
			return "s";
		else if (tp->size == 8)
			return "d";
	} else if (tp->size == 0) {
		return "w";
	}
	abort();
}

void
writeout(void)
{
	Symbol *p;
	Type *tp;
	char *sep, *name;
	int haslabel = 0;

	if (!curfun)
		return;
	if (curfun->kind == SGLOB)
		fputs("export ", stdout);
	printf("function %s %s(", size2stack(&curfun->rtype), symname(curfun));

	/* declare formal parameters */
	for (sep = "", p = locals; p; p = p->next, sep = ",") {
		if ((p->type.flags & PARF) == 0)
			break;
		printf("%s%s %s.val", sep, size2stack(&p->type), symname(p));
	}
	printf("%s)\n{\n", (curfun->type.flags&ELLIPS) ? ", ..." : "");

	/* emit assembler instructions */
	for (pc = prog; pc; pc = pc->next) {
		if (pc->label) {
			haslabel = 1;
			printf("%s\n", symname(pc->label));
		}
		if (!pc->op)
			continue;
		if (pc->flags&BBENTRY && !haslabel)
			printf("%s\n", symname(newlabel()));
		(*optbl[pc->op].fun)();
		if (!pc->label)
			haslabel = 0;
	}

	puts("}");
}

static char *
addr2txt(Addr *a)
{
	switch (a->kind) {
	case SCONST:
		sprintf(buff, "%llu", (unsigned long long) a->u.i);
		return buff;
	case SAUTO:
	case SLABEL:
	case STMP:
	case SGLOB:
	case SEXTRN:
	case SPRIV:
	case SLOCAL:
		return symname(a->u.sym);
	default:
		abort();
	}
}

static void
binary(void)
{
	struct opdata *p = &optbl[pc->op];
	char to[ADDR_LEN], from1[ADDR_LEN], from2[ADDR_LEN];

	strcpy(to, addr2txt(&pc->to));
	strcpy(from1, addr2txt(&pc->from1));
	strcpy(from2, addr2txt(&pc->from2));
	printf("\t%s =%c\t%s\t%s,%s\n", to, p->letter, p->txt, from1, from2);
}

static void
ldir(void)
{
	struct opdata *p = &optbl[pc->op];
	char to[ADDR_LEN], from[ADDR_LEN];
	/* TODO: what type do we use for the size? */

	/* TODO: it is pending */
}

static void
store(void)
{
	struct opdata *p = &optbl[pc->op];
	char to[ADDR_LEN], from[ADDR_LEN];

	strcpy(to, addr2txt(&pc->to));
	strcpy(from, addr2txt(&pc->from1));
	printf("\t\t%s%c\t%s,%s\n", p->txt, p->letter, from, to);
}

static void
unary(void)
{
	struct opdata *p = &optbl[pc->op];
	char to[ADDR_LEN], from[ADDR_LEN];

	strcpy(to, addr2txt(&pc->to));
	strcpy(from, addr2txt(&pc->from1));
	printf("\t%s =%c\t%s\t%s\n", to, p->letter, p->txt, from);
}

static void
call(void)
{
	struct opdata *p = &optbl[pc->op];
	char to[ADDR_LEN], from[ADDR_LEN];
	Symbol *sym = pc->to.u.sym;

	strcpy(to, addr2txt(&pc->to));
	strcpy(from, addr2txt(&pc->from1));
	printf("\t%s =%s\tcall\t%s(",
	       to, size2stack(&sym->type), from);
}

static void
param(void)
{
	Symbol *sym = pc->from2.u.sym;

	printf(optbl[pc->op].txt,
	       size2stack(&sym->type), addr2txt(&pc->from1));
}

static void
ecall(void)
{
	struct opdata *p = &optbl[pc->op];

	puts(p->txt);
}

static void
ret(void)
{
	if (pc->from1.kind == SNONE)
		puts("\t\tret");
	else
		printf("\t\tret\t%s\n", addr2txt(&pc->from1));
}

static void
jmp(void)
{
	printf("\t\tjmp\t%s\n", addr2txt(&pc->from1));
}

static void
branch(void)
{
	char to[ADDR_LEN], from1[ADDR_LEN], from2[ADDR_LEN];

	strcpy(to, addr2txt(&pc->to));
	strcpy(from1, addr2txt(&pc->from1));
	strcpy(from2, addr2txt(&pc->from2));
	printf("\t\tjnz\t%s,%s,%s\n", to, from1, from2);
}

static void
vastart(void)
{
	printf("\t\tvastart %s\n", addr2txt(&pc->from1));
}

static void
vaarg(void)
{
	Symbol *sym = pc->to.u.sym;
	Type *tp = &sym->type;
	char to[ADDR_LEN], from[ADDR_LEN];

	strcpy(to, addr2txt(&pc->to));
	strcpy(from, addr2txt(&pc->from1));
	printf("\t\t%s =%s vaarg %s\n", to, size2asm(tp), from);
}

static void
alloc(void)
{
	Symbol *sym = pc->to.u.sym;
	Type *tp = &sym->type;
	extern Type ptrtype;

	printf("\t%s =%s\talloc%lu\t%lu\n",
	       symname(sym), size2asm(&ptrtype), tp->align+3 & ~3, tp->size);
}

static void
form2local(void)
{
	Symbol *sym = pc->to.u.sym;
	Type *tp = &sym->type;
	char *name = symname(sym);

	printf("\t\tstore%s\t%s.val,%s\n", size2asm(tp), name, name);
}

void
endinit(void)
{
	puts("}");
}

void
getbblocks(void)
{
	Inst *i;

	if (!prog)
		return;

	prog->flags |= BBENTRY;
	for (pc = prog; pc; pc = pc->next) {
		switch (pc->op) {
		case ASBRANCH:
			i = pc->from2.u.sym->u.inst;
			i->flags |= BBENTRY;
		case ASJMP:
			i = pc->from1.u.sym->u.inst;
			i->flags |= BBENTRY;
		case ASRET:
			if (pc->next)
				pc->next->flags |= BBENTRY;
			break;
		}
	}
}
