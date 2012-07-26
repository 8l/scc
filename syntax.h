#ifndef SYNTAX_H
#define SYNTAX_H

extern unsigned char nested_level;


struct node;
struct symbol;

typedef struct symbol *nodeop(struct node *np);

extern void compound(void);
extern struct node *expr(void);
extern unsigned char decl(void);
extern void type_name(void);

extern struct node *leaf(struct symbol *sym);
extern struct node *op1(nodeop *op, struct node *np);
extern struct node *op2(nodeop *op, struct node *np1, struct node *np2);
extern struct node *op3(nodeop *op,
			struct node *np1, struct node *np2, struct node *np3);
#endif
