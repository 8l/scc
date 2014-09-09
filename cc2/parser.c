
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

static Symbol *curfun;
static Node *stack[NR_STACKSIZ], **stackp = stack;
static Node *listexp[NR_EXPRESSIONS], **listp = listexp;
static Node nodepool[NR_NODEPOOL], *newp = nodepool;

static Symbol *localtbl;
static Symbol *globaltbl;

Type l_int8 = {
	.size = 1,
	.align = 2,
	.sign = 1,
	.c_int = 1,
};

Type l_int16 = {
	.size = 2,
	.align = 2,
	.sign = 1,
	.c_int = 1,
};

Type l_int32 = {
	.size = 4,
	.align = 4,
	.sign = 1,
	.c_int = 1,
};

Type l_int64 = {
	.size = 8,
	.align = 8,
	.sign = 1,
	.c_int = 1,
};

Type l_uint8 = {
	.size = 1,
	.align = 2,
	.c_int = 1,
};

Type l_uint16 = {
	.size = 2,
	.align = 2,
	.c_int = 1,
};

Type l_uint32 = {
	.size = 4,
	.align = 4,
	.c_int = 1,
};

Type l_uint64 = {
	.size = 8,
	.align = 8,
	.c_int = 1,
};

static Symbol *
local(char *num)
{
	unsigned i = atoi(num+1);
	static unsigned nr;

	if (i >= NR_INT_IDENT)
		error(EINTNUM);
	if (i > nr) {
		nr = i + 50;
		localtbl = xrealloc(localtbl, nr * sizeof(Symbol));
	}
	return &localtbl[i];
}

static Symbol *
global(char *num)
{
	unsigned i = atoi(num+1);
	static unsigned nr;

	if (i >= NR_EXT_IDENT)
		error(EEXTNUM);
	if (i >= nr) {
		nr = i + 50;
		globaltbl = xrealloc(globaltbl, nr * sizeof(Symbol));
	}
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

static Type *
gettype(char *type)
{
	switch (type[0]) {
	case L_INT8:
		return &l_int8;
	case L_INT16:
		return &l_int16;
	case L_INT32:
		return &l_int32;
	case L_INT64:
		return &l_int64;
	case L_UINT8:
		return &l_uint8;
	case L_UINT16:
		return &l_uint16;
	case L_UINT32:
		return &l_uint32;
	case L_UINT64:
		return &l_uint64;
	case 'F':
		return NULL;
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
	case 'T':
		if (!curfun)
			goto global;
		op = MEM;
		goto local;
	case 'P':
		op = AUTO;
		goto local;
	case 'A':
		op = AUTO;
		goto local;
	case 'R':
		op = REG;
	local:
		sym = local(token);
		break;
	case 'X':
		/* TODO */
	case 'G':
	global:
		sym = global(token);
		op = MEM;
		break;
	}

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
	np->type = gettype(token+1);
	np->u.imm = atoi(token+2);
	np->left = np->right = NULL;
	push(np);
}

static void
unary(char *token)
{
	Node *np = newnode();

	np->right = NULL;
	np->left = pop();
	np->type = gettype(token+1);
	np->op = token[0];
	push(np);
}

static void
operator(char *token)
{
	Node *np = newnode();

	np->right = pop();
	np->left = pop();
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

static void
increment(char *token)
{
	Node *np = newnode();

	np->right = pop();
	np->left = pop();
	np->type = gettype(token+2);
	np->op = token[0];
	switch (np->subop = token[1]) {
	case '-': case '+':
		push(np);
		break;
	default:
		error(ESYNTAX);
	}
}

static void
assignment(char *token)
{
	Node *np = newnode();

	np->right = pop();
	np->left = pop();
	np->op = *token;
	switch (*++token) {
	case OADD: case OSUB: case OINC:  case OMOD: case ODIV:
	case OSHL: case OSHR: case OBAND: case OBOR: case OBXOR:
		np->subop = *++token;
	default:
		np->type = gettype(token);
		break;
	}
	push(np);
}

static void (*optbl[])(char *) = {
	['+'] = operator,
	['%'] = operator,
	['-'] = operator,
	['*'] = operator,
	['/'] = operator,
	['l'] = operator,
	['r'] = operator,
	['&'] = operator,
	['|'] = operator,
	['^'] = operator,
	[':'] = assignment,
	[';'] = increment,
	['A'] = variable,
	['T'] = variable,
	['G'] = variable,
	['L'] = label,
	['#'] = immediate,
	['@'] = unary,
	['a'] = unary,
	['<'] = operator,
	['>'] = operator,
	[']'] = operator,
	['['] = operator,
	['='] = operator,
	['!'] = operator,
	['y'] = operator,
	['o'] = operator,
	['_'] = unary,
	['~'] = unary,
	[','] = operator,
	['\177'] = NULL
};

static void
expression(char *token)
{
	Node *np;
	void (*fun)(char *);

	if (!curfun)
		error(ESYNTAX);

	do {
		if ((fun = optbl[token[0]]) == NULL)
			error(ESYNTAX);
		(*fun)(token);
	} while (token = strtok(NULL, "\t"));

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
	char class = token[0], *type, *name, *brace;
	Symbol *sym;

	switch (class) {
	case 'P':
		if (!curfun)
			error(ESYNTAX);
		sym = local(token);
		sym->next = curfun->u.f.pars;
		curfun->u.f.pars = sym;
		break;
	case 'T':
		if (!curfun) {
			sym = global(token);
			break;
		}
	case 'R': case 'A':
		if (!curfun)
			error(ESYNTAX);
		sym = local(token);
		sym->next = curfun->u.f.vars;
		curfun->u.f.vars = sym;
		break;
	case 'X':
		sym = global(token);
		sym->extrn = 1;
		break;
	case 'G':
		sym = global(token);
		sym->public = 1;
		break;
	}

	type = strtok(NULL, "\t");
	if (type[0] == 'F') {
		sym->type = FUN;
	} else {
		sym->type = VAR;
		sym->u.v.sclass = class;
		sym->u.v.type = gettype(type);
	}
	if (name = strtok(NULL, "\t"))
		sym->name = xstrdup(name);
	if (brace = strtok(NULL, "\t")) {
		if (brace[0] != '{' || type[0] != 'F' || curfun)
			error(ESYNTAX);
		sym->u.f.vars = NULL;
		sym->u.f.pars = NULL;
		curfun = sym;
		listp = listexp;
		newp = nodepool;
	}
}

static void
deflabel(char *token)
{
	Symbol *sym;

	if (!curfun)
		error(ESYNTAX);
	sym = local(token);
	sym->type = LABEL;
	sym->u.l.addr = listp - listexp;
}

static void
endfunction(char *token)
{
	listp = NULL;
	genstack(curfun);
	genaddable(listexp);
	cgen(curfun, listexp);
	curfun = NULL;
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
			fun = deflabel;
			break;
		case '\t':
			fun = expression;
			break;
		case 'S':
			/* struct */
			break;
		case 'T': case 'G': case 'A': case 'R': case 'P':
			fun = declaration;
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
