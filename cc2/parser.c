
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

static Node *stack[NR_STACKSIZ], **stackp = stack;
static Node *listexp[NR_EXPRESSIONS], **listp = &listexp[0];
static Node nodepool[NR_NODEPOOL], *newp = nodepool;

static Symbol *localtbl;
static Symbol *globaltbl;

static Symbol *
local(void)
{
	unsigned i;
	static unsigned nr;

	scanf("%u", &i);
	if (i >= NR_INT_IDENT)
		error(EINTNUM);
	if (i >= nr)
		localtbl = xrealloc(localtbl, i+1);
	return &localtbl[i];
}

static Symbol *
global(void)
{
	unsigned i;
	static unsigned nr;

	scanf("%u", &i);
	if (i >= NR_EXT_IDENT)
		error(EEXTNUM);
	if (i >= nr)
		globaltbl = xrealloc(globaltbl, i+1);
	return &globaltbl[i];
}

static Node *
newnode(void)
{
	if (newp == &nodepool[NR_NODEPOOL])
		error(ENODEOV);
	return newp++;
}

static void
push(Node *np)
{
	if (stackp == &stack[NR_STACKSIZ])
		error(ESTACKO);
	*stackp++ = np;
}

static Node *
pop(void)
{
	if (stackp == stack)
		error(ESTACKU);
	return *--stackp;
}

static void
link(Node *np)
{
	if (listp == &listexp[NR_EXPRESSIONS])
		error(EEXPROV);
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
		error(ETYPERR);
	}
}

static void
variable(char op)
{
	Symbol *sym;
	Node *np = newnode();

	switch (op) {
	case 'A':
		sym = local();
		op = AUTO;
		break;
	case 'R':
		sym = local();
		op = REGISTER;
		break;
	case 'T':
		sym = local();
		op = STATIC;
		break;
	case 'G':
		sym = global();
		op = STATIC;
		break;
	}

	np->u.sym = sym;
	np->op = op;
	np->type = sym->u.v.type;
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
		error(EEXPBAL);
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
			error(ESYNTAX);
		if ((fun = optbl[c]) == NULL)
			error(ESYNTAX);
		(*fun)(c);
	} while ((c = getchar()) == '\t');

	if (c != '\n')
		error(ESYNTAX);
	link(getroot());
}

static void
declaration(char sclass, char islocal)
{
	Symbol *sym = (islocal) ? local() : global();

	getchar(); /* skip tab */
	sym->u.v.storage = sclass;
	sym->u.v.type = gettype();
	if (getchar() != '\n')
		error(ESYNTAX);
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
	Symbol *sym = local();

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
			declaration(c, 1);
			break;
		case '}':
			chop();
			return;
		default:
			error(ESYNTAX);
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
			declaration(c, 0);
			break;
		case 'X':
			function();
			genaddable(listexp[0]);
			break;
		case EOF:
			return;
			break;
		default:
			error(ESYNTAX);
		}
	}
}

