
#include <ctype.h>
#include <stdio.h>

#include <cc.h>
#include <sizes.h>

#define NR_STACKSIZ 32
#define NR_NODEPOOL 128
#define NR_EXPRESSIONS 64
#define NR_SYMBOLS 1024

typedef struct {
	char vartype;
	char storage;
} Symbol;

typedef struct node {
	char op;
	char type;
	union {
		short id;
		int imm;
	} u;
	struct node *left, *right;
} Node;

static Node *stack[NR_STACKSIZ], **stackp = stack;
static Node *listexp[NR_EXPRESSIONS], **listp = &listexp[0];
static Node nodepool[NR_NODEPOOL], *newp = nodepool;
static Symbol symtbl[NR_SYMBOLS];

extern void esyntax(void);

static short
getid(void)
{
	int i;

	scanf("%d", &i);
	if (i < 0 || i >= NR_SYMBOLS)
		esyntax();
	return i;
}

static Node *
newnode(void)
{
	if (newp == &nodepool[NR_NODEPOOL])
		esyntax();
	return newp++;
}

static void
push(Node *np)
{
	if (stackp == &stack[NR_STACKSIZ])
		esyntax();
	*stackp++ = np;
}

static Node *
pop(void)
{
	if (stackp == stack)
		esyntax();
	return *--stackp;
}

static void
link(Node *np)
{
	if (listp == &listexp[NR_EXPRESSIONS])
		esyntax();
	*listp++ = np;
}

static char
gettype(void)
{
	char t;

	switch (t = getchar()) {
	case L_INT16: case L_INT8:
		return t;
	default:
		esyntax();
	}
}

static void
variable(char op)
{
	Node *np = newnode();
	short id = getid();

	np->op = op;
	np->u.id = id;
	np->type = symtbl[id].vartype;
	np->left = np->right = NULL;
	push(np);
}

static void
immediate(char op)
{
	Node *np = newnode();

	np->op = '#';
	np->type = L_INT;
	scanf("%d", &np->u.imm);
	np->left = np->right = NULL;
	push(np);
}

static void
operator(char op)
{
	Node *np = newnode();

	np->left = pop();
	np->right = pop();
	np->type = gettype();
	np->op = op;
	push(np);
}

static Node *
getroot(void)
{
	Node *np = *--stackp;
	if (stackp != stack)
		esyntax();
	return np;
}

static void (*optbl[])(char) = {
	['+'] = operator,
	['-'] = operator,
	['*'] = operator,
	['/'] = operator,
	['A'] = variable,
	['T'] = variable,
	['G'] = variable,
	['#'] = immediate,
	['\177'] = NULL
};

static void
expression(void)
{
	int c;
	void (*fun)(char);

	do {
		if (!isprint(c = getchar()))
			esyntax();
		if ((fun = optbl[c]) == NULL)
			esyntax();
		(*fun)(c);
	} while ((c = getchar()) == '\t');

	if (c != '\n')
		esyntax();
	link(getroot());
}

static void
declaration(char sclass)
{
	Symbol *sym = &symtbl[getid()];

	getchar(); /* skip tab */
	sym->storage = sclass;
	sym->vartype = gettype();
	if (getchar() != '\n')
		esyntax();
}

int
parse(void)
{
	int c;

	while ((c = getchar()) != EOF) {
		switch (c) {
		case '\t':
			expression();
			break;
		case 'L':
			/* label */
			break;
		case 'S':
			/* struct */
			break;
		case 'T': case 'A': case 'G': case 'R':
			declaration(c);
			break;
		default:
			esyntax();
			break;
		}
	}
}

