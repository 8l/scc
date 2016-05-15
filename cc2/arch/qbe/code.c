
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "../../cc2.h"
#include "../../../inc/sizes.h"

#define ADDR_LEN (INTIDENTSIZ+64)

static void binary(void), unary(void), store(void), jmp(void), ret(void),
            branch(void);

static struct opdata {
	void (*fun)(void);
	char *txt;
	char letter;
} optbl [] = {
	[ASLDB]   =  {.fun = unary,  .txt = "load", .letter = 'b'},
	[ASLDH]   =  {.fun = unary,  .txt = "load", .letter = 'h'},
	[ASLDW]   =  {.fun = unary,  .txt = "load", .letter = 'w'},
	[ASLDL]   =  {.fun = unary,  .txt = "load", .letter = 'l'},
	[ASLDS]   =  {.fun = unary,  .txt = "load", .letter = 's'},
	[ASLDD]   =  {.fun = unary,  .txt = "load", .letter = 'd'},

	[ASSTB]   =  {.fun = store,  .txt = "store", .letter = 'b'},
	[ASSTH]   =  {.fun = store,  .txt = "store", .letter = 'h'},
	[ASSTW]   =  {.fun = store,  .txt = "store", .letter = 'w'},
	[ASSTL]   =  {.fun = store,  .txt = "store", .letter = 'l'},
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
	[ASSHRW]  =  {.fun = binary, .txt = "shrs", .letter = 'w'},
	[ASUSHRW] =  {.fun = binary, .txt = "shrz", .letter = 'w'},
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
	[ASSHRL]  =  {.fun = binary, .txt = "shrs", .letter = 'l'},
	[ASUSHRL] =  {.fun = binary, .txt = "shrz", .letter = 'l'},
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
	[ASEXTWL] =  {.fun = unary, .txt = "extsh", .letter = 'l'},
	[ASUEXTWL]=  {.fun = unary, .txt = "extuh", .letter = 'l'},

	[ASSTOL] = {.fun = unary, .txt = "stosi", .letter = 'l'},
	[ASSTOW] = {.fun = unary, .txt = "stosi", .letter = 'w'},
	[ASDTOL] = {.fun = unary, .txt = "dtosi", .letter = 'l'},
	[ASDTOW] = {.fun = unary, .txt = "dtosi", .letter = 'w'},

	[ASSWTOD] = {.fun = unary, .txt = "swtof", .letter = 'd'},
	[ASSWTOS] = {.fun = unary, .txt = "swtof", .letter = 's'},
	[ASSLTOD] = {.fun = unary, .txt = "sltof", .letter = 'd'},
	[ASSLTOS] = {.fun = unary, .txt = "sltof", .letter = 's'},

	[ASEXTS] = {.fun = unary, .txt = "exts", .letter = 'd'},
	[ASSLTOS]= {.fun = unary, .txt = "truncd", .letter = 's'},

	[ASBRANCH] = {.fun = branch},
	[ASJMP]  = {.fun = jmp},
	[ASRET]  = {.fun = ret},
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
	} else {
		switch (tp->size) {
		case 0:
			return "";
		case 1:
			return "b";
		case 2:
			return "h";
		case 4:
			return "w";
		case 8:
			return "l";
		default:
			abort();
		}
	}
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
	printf("\tz\t%llu\n}\n", (unsigned long long) sym->type.size);
}

void
defpar(Symbol *sym)
{
	sym->type.flags |= PARF;
}

void
defvar(Symbol *sym)
{
}

void
data(Node *np)
{
	printf("\t%s\t", size2asm(&np->type));
	emittree(np);
	putchar(',');
	putchar('\n');
}

static void
alloc(Symbol *sym)
{
	Type *tp = &sym->type;

	printf("\t%s %s=\talloc%lld\t%lld\n",
	       symname(sym), size2asm(tp),
	       (long long) tp->size, (long long) tp->align);
}

void
writeout(void)
{
	Symbol *p;
	Type *tp;
	char *sep, *name;

	if (curfun->kind == SGLOB)
		fputs("export ", stdout);
	printf("function %s %s(", size2asm(&rtype), symname(curfun));

	/* declare formal parameters */
	for (sep = "", p = locals; p; p = p->next, sep = ",") {
		if ((p->type.flags & PARF) == 0)
			break;
		printf("%s%s %s.val", sep, size2asm(&p->type), symname(p));
	}
	puts(")\n{");

	/* allocate stack space for parameters */
	for (p = locals; p && (p->type.flags & PARF) != 0; p = p->next)
		alloc(p);

	/* allocate stack space for local variables) */
	for ( ; p && p->id != TMPSYM; p = p->next) {
		if (p->kind != SLABEL)
			alloc(p);
	}
	/* store formal parameters in parameters */
	for (p = locals; p; p = p->next) {
		tp = &p->type;
		if ((tp->flags & PARF) == 0)
			break;
		name = symname(p);
		printf("\t\tstore%s\t%s.val,%s\n", size2asm(tp), name, name);
	}

	/* emit assembler instructions */
	for (pc = prog; pc; pc = pc->next) {
		if (pc->label)
			printf("%s:\n", symname(pc->label));
		if (pc->op)
			(*optbl[pc->op].fun)();
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
	printf("\t%s %c=\t%s\t%s,%s\n", to, p->letter, p->txt, from1, from2);
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
	printf("\t%s %c=\t%s\t%s\n", to, p->letter, p->txt, from);
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

void
endinit(void)
{
	puts("}");
}
