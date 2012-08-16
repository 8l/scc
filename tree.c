
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "cc.h"
#include "syntax.h"
#include "symbol.h"

struct node {
	unsigned char op;
};

struct node_op1 {
	struct node base;
	struct node *infix;
};

struct node_op2 {
	struct node base;
	struct node *left;
	struct node *rigth;
};

struct node_op3 {
	struct node base;
	struct node *left;
	struct node *infix;
	struct node *rigth;
};

struct node_sym {
	struct node base;
	struct symbol *sym;
};

struct node_comp {
	struct node base;
	uint8_t nr, alloc;
	struct node **body;
};

struct node *
nodesym(struct symbol *sym)
{
	register struct node_sym *np = xmalloc(sizeof(*np));

	np->base.op = OSYM;
	np->sym = sym;
	return (struct node *) np;
}

struct node *
node3(unsigned char op, struct node *l, struct node *i, struct node *r)
{
	register struct node_op3 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->infix = i;
	np->rigth = r;

	return (struct node *) np;
}

struct node *
node2(unsigned char op, struct node *l, struct node *r)
{
	register struct node_op2 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->rigth = r;

	return (struct node *) np;
}

struct node *
node1(unsigned char op, struct node *i)
{
	register struct node_op1 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->infix = i;

	return (struct node *) np;
}

struct node *
nodecomp(void)
{
	register struct node_comp *np = xmalloc(sizeof(*np));

	np->base.op = OCOMP;
	np->alloc = np->nr = 0;
	np->body = NULL;

	return (struct node *) np;
}

struct node *
addstmt(struct node *p, struct node *stmt)
{
	register uint8_t nr, alloc;
	register struct node_comp *np = (struct node_comp *) p;

	assert(np && np->base.op == OCOMP);
	nr = ++np->nr, alloc = np->alloc;

#define alloc_nr(x) ((((x)+16)*3)/2)
	if (nr > alloc) {
		alloc = alloc_nr(nr);
		np->body = xrealloc(np->body, alloc * sizeof(*np->body));
	}
#undef alloc_nr

	np->body[nr - 1] = stmt;
	np->alloc = alloc;
	return p;
}

void
prtree(register struct node *np)
{
	static struct optab {
		unsigned char nchild;
		const char *txt;
	} *bp, optab [] = {
		[OCALL] = {1, "()"},
		[OARY] = {2, "[]"},
		[OFIELD] = {2, "."},
		[OPTR] = {2, "->"},
		[OPOSTINC] = {1, ".++"},
		[OPOSTDEC] = {1, ".--"},
		[OPREINC] = {1, "++."},
		[OPREDEC] = {1, "--."},
		[OADDR] = {1, "&."},
		[OINDIR] = {1, "[*]"},
		[OMINUS] = {1, "-."},
		[OPLUS] = {1, "+."},
		[OCPL] = {1, "~"},
		[ONEG] = {1, "!"},
		[OMUL] = {2, "*"},
		[ODIV] = {2, "/"},
		[OMOD] = {2, "%"},
		[OADD] = {2, "+"},
		[OSUB] = {2, "-"},
		[OSHL] = {2, "<<"},
		[OSHR] = {2, ">>"},
		[OLT] = {2, "<"},
		[OGT] = {2, ">"},
		[OGE] = {2, ">="},
		[OLE] = {2, "<="},
		[OEQ] = {2, "=="},
		[ONE] = {2, "!="},
		[OBAND] = {2, "&"},
		[OBXOR] = {2, "^"},
		[OBOR] = {2, "|"},
		[OAND] = {2, "&&"},
		[OOR] = {2, "||"},
		[OTERN] = {3, "?"},
		[OASSIGN] = {2, "="},
		[OA_MUL] = {2, "*="},
		[OA_DIV] = {2, "/="},
		[OA_MOD] = {2, "%="},
		[OA_ADD] = {2, "+="},
		[OA_SUB] = {2, "-="},
		[OA_SHL] = {2, "<<="},
		[OA_SHR] = {2, ">>="},
		[OA_AND] = {2, "&="},
		[OA_XOR] = {2, "^="},
		[OA_OR] = {2, "|="},
		[OSYM] = {0, "sym"},
		[OCOMP] = {255, "comp"}
	};

	assert(np && np->op < ARRAY_SIZE(optab));
	bp = &optab[np->op];
	if (bp->nchild)
		printf("(%s ", bp->txt);

	switch (bp->nchild) {
	case 0: {
		register struct symbol *sym = ((struct node_sym *) np)->sym;
		printf(" %s", (sym->name) ? sym->name : ".");
		return;
	}
	case 1:
		prtree(((struct node_op1 *) np)->infix);
		break;
	case 2:
		prtree(((struct node_op2 *) np)->left);
		prtree(((struct node_op2 *) np)->rigth);
		break;
	case 3:
		prtree(((struct node_op3 *) np)->left);
		prtree(((struct node_op3 *) np)->infix);
		prtree(((struct node_op3 *) np)->rigth);
		break;
	case 255: {
		register struct node **bp, **lim;

		bp = ((struct node_comp *) np)->body;
		lim = bp + ((struct node_comp *) np)->nr;
		while (bp < lim)
			prtree(*bp++);
		break;
	}
	}
	putchar(')');
}
