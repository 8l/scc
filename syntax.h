#ifndef SYNTAX_H
#define SYNTAX_H

#if ! __bool_true_false_are_defined
# include <stdbool.h>
#endif

extern unsigned char curctx;
extern uint8_t namespace;

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
struct ctype;

extern struct node *expr(void), *extdecl(void), *decl(void),
	*typename(void), *function(void);

extern struct node *node(unsigned char op, struct node *l, struct node *r);
extern struct node *nodesym(struct symbol *sym);
extern bool walk(register struct node *np, bool (*fun)(struct node *));

#endif
