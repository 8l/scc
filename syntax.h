#ifndef SYNTAX_H
#define SYNTAX_H

extern unsigned char curctx;

enum opcode {
	OARY, OCALL, OFIELD, OPTR, OPOSTINC, OPOSTDEC,
	OPREINC, OPREDEC, OADDR, OINDIR, OMINUS, OPLUS,
	OCPL, ONEG, OMUL, ODIV, OMOD, OADD, OSUB, OSHL,
	OSHR, OLT, OGT, OGE, OLE, OEQ, ONE, OBAND, OBXOR,
	OBOR, OAND, OOR, OTERN, OASSIGN, OA_MUL, OA_DIV,
	OA_MOD, OA_ADD, OA_SUB, OA_SHL, OA_SHR, OA_AND,
	OA_XOR, OA_OR, OSYM, OCOMP, OSWITCH, OIF, OFOR,
	OFEXP, ODO, OWHILE, OLABEL, OGOTO, OBREAK, OCONT,
	ORETURN, OCASE, ODEFAULT, OFTN, ODEF, O2EXP
};

struct node;
struct symbol;

struct compound {
	struct node *tree;
	struct node_op2 *last;
};

extern struct node *expr(void);
extern struct node *decl(void);
extern void type_name(void);
extern struct node *function(struct symbol *sym);

extern struct node *node(unsigned char op, struct node *l, struct node *r);
extern struct node *nodesym(struct symbol *sym);
extern struct node *addstmt(struct compound *p, struct node *np);
extern struct node *addstmt(struct compound *p, struct node *np);

extern void prtree(register struct node *np);

#endif
