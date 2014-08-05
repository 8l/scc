
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include <cc.h>
#include <sizes.h>

#include "cc2.h"

#define STR(x) XSTR(x)
#define XSTR(x) #x

#define NR_STACKSIZ 32
#define NR_NODEPOOL 128
#define NR_EXPRESSIONS 64
#define NR_SYMBOLS 1024

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
	np->type = symtbl[id].u.v.type;
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

static void
label(char op)
{
	Node *np = newnode();

	np->left = np->right = NULL;
	np->op = 'L';
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
	['L'] = label,
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
	sym->u.v.storage = sclass;
	sym->u.v.type = gettype();
	if (getchar() != '\n')
		esyntax();
}

static void
chop(void)
{
	int c;

	while ((c = getchar()) != EOF && c != '\n')
		/* nothing */;
}

static void
deflabel(void)
{
	Symbol *sym = &symtbl[getid()];

	sym->u.l.addr = listp - listexp;
	chop();
}

static void
function(void)
{
	int c;
	char name[IDENTSIZ + 1];

	scanf("%" STR(IDENTSIZ) "s", name);
	chop();
	for (;;) {
		switch (c = getchar()) {
		case '\t':
			expression();
			break;
		case 'L':
			deflabel();
			break;
		case 'S':
			/* struct */
			break;
		case 'T': case 'A': case 'R':
			declaration(c);
			break;
		case '}':
			chop();
			return;
		default:
			esyntax();
			break;
		}
	}
}

void
parse(void)
{
	int c;
	void genaddr(Node *np);

	for (;;) {
		switch (c = getchar()) {
		case 'G':
			declaration(c);
			break;
		case 'X':
			function();
			genaddr(listexp[0]);
			break;
		case EOF:
			return;
			break;
		default:
			esyntax();
		}
	}
}

