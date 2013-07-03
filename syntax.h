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
	ORETURN, OCASE, ODEFAULT, OFTN, ODEF
};

struct node;
struct symbol;

extern struct node *expr(void);
extern struct node *decl(void);
extern void type_name(void);
extern struct node *function(struct symbol *sym);

extern struct node *node3(unsigned char op,
			  struct node *l, struct node *i, struct node *r);
extern struct node *node2(unsigned char op, struct node *l, struct node *r);
extern struct node *node1(unsigned char op, struct node *i);

extern struct node *nodesym(struct symbol *sym);
extern struct node *nodecomp(void);
extern struct node *addstmt(struct node *np, struct node *stmt);

extern void prtree(register struct node *np);

#endif
