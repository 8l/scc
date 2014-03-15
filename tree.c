
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cc.h"
#include "syntax.h"
#include "symbol.h"

struct node {
	unsigned char op;
};

struct node_op2 {
	struct node base;
	struct node *left;
	struct node *right;
};

struct nodesym {
	struct node base;
	struct symbol *sym;
};


struct node *
nodesym(struct symbol *sym)
{
	register struct nodesym *np = xmalloc(sizeof(*np));

	np->base.op = OSYM;
	np->sym = sym;
	return (struct node *) np;
}

struct node *
node(unsigned char op, struct node *l, struct node *r)
{
	register struct node_op2 *np = xmalloc(sizeof(*np));

	np->base.op = op;
	np->left = l;
	np->right = r;

	return (struct node *) np;
}

struct node *
addstmt(struct compound *p, struct node *np)
{
	if (!p->tree) {
		p->tree = node(OCOMP, NULL, NULL);
		p->last = (struct node_op2 *) p->tree;
		p->last->right = np;
	} else {
		p->last = (struct node_op2 *)
		   (p->last->left = node(O2EXP, NULL, np));
	}

	return p->tree;
}

bool
walk(register struct node *np, bool (*fun)(struct node *))
{
	struct node_op2 *p;

	if (!np || np->op == OSYM)
		return 1;

	p = (struct node_op2 *) np;
	return (*fun)(np) && walk(p->left, fun) && walk(p->right, fun);
}

void
prtree(register struct node *np)
{
	static unsigned char indent;
	register unsigned char i;
	static char *optab[] = {
		[OCALL] = "()",
		[OARY] = "[]",
		[OFIELD] = ".",
		[OPTR] = "->",
		[OPOSTINC] = ".++",
		[OPOSTDEC] = ".--",
		[OPREINC] = "++.",
		[OPREDEC] = "--.",
		[OADDR] = "&.",
		[OINDIR] = "[*]",
		[OMINUS] = "-.",
		[OPLUS] = "+.",
		[OCPL] = "~",
		[ONEG] = "!",
		[OMUL] = "*",
		[ODIV] = "/",
		[OMOD] = "%",
		[OADD] = "+",
		[OSUB] = "-",
		[OSHL] = "<<",
		[OSHR] = ">>",
		[OLT] = "<",
		[OGT] = ">",
		[OGE] = ">=",
		[OLE] = "<=",
		[OEQ] = "==",
		[ONE] = "!=",
		[OBAND] = "&",
		[OBXOR] = "^",
		[OBOR] = "|",
		[OAND] = "&&",
		[OOR] = "||",
		[OTERN] = "?",
		[OASSIGN] = "=",
		[OA_MUL] = "*=",
		[OA_DIV] = "/=",
		[OA_MOD] = "%=",
		[OA_ADD] = "+=",
		[OA_SUB] = "-=",
		[OA_SHL] = "<<=",
		[OA_SHR] = ">>=",
		[OA_AND] = "&=",
		[OA_XOR] = "^=",
		[OA_OR] = "|=",
		[OSYM] = "sym",
		[OCOMP] = "comp",
		[OSWITCH] = "switch",
		[OIF] = "if",
		[OFOR] = "for",
		[OFEXP] = "efor",
		[ODO] = "do",
		[OWHILE] = "while",
		[OLABEL] = "label",
		[OGOTO] = "goto",
		[OBREAK] = "break",
		[OCONT] = "cont",
		[ORETURN] = "return",
		[OCASE] = "case",
		[ODEFAULT] = "default",
		[OFTN] = "function",
		[O2EXP] = ":",
		[ODEF] = "def"
	};

	if (!np) {
		fputs(" nil", stdout);
		return;
	}
	if (np->op == OSYM) {
		const char *s = ((struct nodesym *) np)->sym->name;

		printf(" %s", s ? s : ".");
		return;
	}

	putchar('\n');
	for (i = indent; i != 0; --i)
		putchar(' ');

	indent += 2;
	assert(np->op < ARRAY_SIZE(optab));
	printf("(%s", optab[np->op]);
	prtree(((struct node_op2 *)np)->right);
	prtree(((struct node_op2 *)np)->left);
	putchar(')');
	indent -= 2;
}

