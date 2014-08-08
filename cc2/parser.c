
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cc.h>
#include <sizes.h>

#include "cc2.h"

#define MAXLINE 200
#define NR_STACKSIZ 32
#define NR_NODEPOOL 128
#define NR_EXPRESSIONS 64

static char funbody;
static Symbol *curfun;
static Node *stack[NR_STACKSIZ], **stackp = stack;
static Node *listexp[NR_EXPRESSIONS], **listp = listexp;
static Node nodepool[NR_NODEPOOL], *newp = nodepool;

static Symbol *localtbl;
static Symbol *globaltbl;

static Symbol *
local(char *num)
{
	unsigned i = atoi(num+1);
	static unsigned nr;

	if (i >= NR_INT_IDENT)
		error(EINTNUM);
	if (i >= nr)
		localtbl = xrealloc(localtbl, i+1);
	return &localtbl[i];
}

static Symbol *
global(char *num)
{
	unsigned i = atoi(num+1);
	static unsigned nr;

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

static char
gettype(char *type)
{
	switch (type[0]) {
	case L_INT16: case L_INT8:
		return type[0];
	default:
		error(ETYPERR);
	}
}

static void
variable(char *token)
{
	Symbol *sym;
	char op, public = 0;
	Node *np = newnode();

	switch (token[0]) {
	case 'A':
		sym = local(token);
		op = AUTO;
		break;
	case 'R':
		sym = local(token);
		op = REGISTER;
		break;
	case 'T':
		sym = (funbody) ? local(token) : global(token);
		op = STATIC;
		break;
	case 'G':
		sym = global(token);
		op = STATIC;
		public = 1;
		break;
	}

	sym->public = public;
	np->u.sym = sym;
	np->op = op;
	np->type = sym->u.v.type;
	np->left = np->right = NULL;
	push(np);
}

static void
immediate(char *token)
{
	Node *np = newnode();

	np->op = CONST;
	/* TODO: deal with constant non integer */
	np->type = L_INT;
	np->u.imm = atoi(token+1);
	np->left = np->right = NULL;
	push(np);
}

static void
operator(char *token)
{
	Node *np = newnode();

	np->left = pop();
	np->right = pop();
	np->type = gettype(token+1);
	np->op = token[0];
	push(np);
}

static void
label(char *token)
{
	Node *np = newnode();

	np->left = np->right = NULL;
	np->op = LABEL;
	np->u.sym = local(token);
	push(np);
}

static void (*optbl[])(char *) = {
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
expression(char *token)
{
	Node *np;
	void (*fun)(char *);

	do {
		if ((fun = optbl[token[0]]) == NULL)
			error(ESYNTAX);
		(*fun)(token);
	} while ((token = strtok(NULL, "\t")) != NULL);

	np = pop();
	if (stackp != stack)
		error(EEXPBAL);
	if (listp == &listexp[NR_EXPRESSIONS])
		error(EEXPROV);
	*listp++ = np;
}

static void
declaration(char *token)
{
	char class = token[0];
	Symbol *sym;

	sym = (class == 'G') ? global(token) : local(token);

	sym->u.v.storage = class;
	sym->u.v.type = gettype(strtok(NULL, "\t"));
}

static void
deflabel(char *token)
{
	Symbol *sym;

	sym = local(token);
	sym->u.l.addr = listp - listexp;
}

static void
function(char *token)
{
	funbody = 1;
	curfun = global(token);
	if ((token = strtok(NULL, "\t")) == NULL)
		error(ESYNTAX);
	curfun->u.f.name = xstrdup(token);
	listp = listexp;
	newp = nodepool;
}

static void
endfunction(char *token)
{
	funbody = 0;
	listp = NULL;
	genaddable(listexp);
	cgen(curfun, listexp);
}

void
parse(void)
{
	void (*fun)(char *tok);
	uint8_t len;
	int c;
	char line[MAXLINE];

	for (;;) {
		switch (c = getchar()) {
		case 'L':
			if (!funbody)
				goto syntax_error;
			fun = deflabel;
			break;
		case '\t':
			if (!funbody)
				goto syntax_error;
			fun = expression;
			break;
		case 'S':
			/* struct */
			break;
		case 'T': case 'A': case 'R':
			if (!funbody)
				goto syntax_error;
			fun = declaration;
			break;
		case 'G':
			if (funbody)
				goto syntax_error;
			fun = declaration;
			break;
		case 'X':
			if (funbody)
				goto syntax_error;
			fun = function;
			break;
		case '}':
			fun = endfunction;
			break;
		case EOF:
			goto found_eof;
		default:
			goto syntax_error;
		}

		ungetc(c, stdin);
		if (!fgets(line, sizeof(line), stdin))
			break;
		len = strlen(line);
		if (line[len-1] != '\n')
			error(ELNLINE);
		line[len-1] = '\0';
		(*fun)(strtok(line, "\t"));
	}

found_eof:
	if (ferror(stdin))
		error(EFERROR, strerror(errno));
	return;

syntax_error:
	error(ESYNTAX);
}
