
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/cc.h"
#include "../inc/sizes.h"

#include "cc2.h"

#define MAXLINE 200
#define NR_STACKSIZ 32
#define NR_NODEPOOL 128
#define NR_EXPRESSIONS 64

enum {
	LOCAL, GLOBAL, PARAMETER
};

Symbol *curfun;
static Node *stack[NR_STACKSIZ], **stackp;
static Node *listexp[NR_EXPRESSIONS], **listp;
static Node nodepool[NR_NODEPOOL], *newp;


static Type Funct = {
	.letter = L_FUNCTION,
};

static Type l_int8 = {
	.letter = L_INT8,
	.size = 1,
	.align = 2,
	.flags = SIGNF | INTF
};

static Type l_int16 = {
	.letter = L_INT16,
	.size = 2,
	.align = 2,
	.flags = SIGNF | INTF

};

static Type l_int32 = {
	.letter = L_INT32,
	.size = 4,
	.align = 4,
	.flags = SIGNF | INTF

};

static Type l_int64 = {
	.letter = L_INT64,
	.size = 8,
	.align = 8,
	.flags = SIGNF | INTF

};

static Type l_uint8 = {
	.letter = L_UINT8,
	.size = 1,
	.align = 2,
	.flags =  INTF
};

static Type l_uint16 = {
	.letter = L_UINT16,
	.size = 2,
	.align = 2,
	.flags =  INTF
};

static Type l_uint32 = {
	.letter = L_UINT32,
	.size = 4,
	.align = 4,
	.flags =  INTF
};

static Type l_uint64 = {
	.letter = L_UINT64,
	.size = 8,
	.align = 8,
	.flags =  INTF
};

static void cast(char *), operator(char *), assignment(char *), increment(char *),
            globvar(char *), localvar(char *), paramvar(char *), label(char *),
            immediate(char *), unary(char *), oreturn(char *);

/*TODO: Remove hardcoded symbols */

static void (*optbl[])(char *) = {
	[L_INT8] = cast,
	[L_INT16] = cast,
	[L_INT32] = cast,
	[L_INT64] = cast,
	[L_UINT8] = cast,
	[L_UINT16] = cast,
	[L_UINT32] = cast,
	[L_UINT64] = cast,
	[L_BOOL] = cast,
	[L_FLOAT] = cast,
	[L_DOUBLE] = cast,
	[L_LDOUBLE] = cast,
	[L_POINTER] = cast,
	[L_VOID] = cast,
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
	['Y'] = globvar,
	['A'] = localvar,
	['K'] = localvar,
	['T'] = localvar,
	['G'] = globvar,
	['P'] = paramvar,
	['L'] = label,
	['#'] = immediate,
	['@'] = unary,
	['a'] = unary,
	['<'] = operator,
	['>'] = operator,
	[']'] = operator,
	['['] = operator,
	['='] = operator,
	['!'] = unary,
	['y'] = oreturn,
	['j'] = NULL,
	['o'] = operator,
	['_'] = unary,
	['~'] = unary,
	[','] = operator,
	['\177'] = NULL
};

static void
prnode(Node *np)
{
	if (np->left)
		prnode(np->left);
	if (np->right)
		prnode(np->right);
	fprintf(stderr, "\t%c%c", np->op, np->type.letter);
}

void
prtree(Node *np)
{
	prnode(np);
	putc('\n', stderr);
}

void
apply(Node *(*fun)(Node *))
{
	Node **list, *np;

	for (list = curfun->u.f.body; np = *list; ++list)
		*list++ = (*fun)(np);
}

static Symbol *
parameter(char *num)
{
	static Symbol tbl[NR_FUNPARAM];
	Symbol *sym;
	unsigned i = atoi(num);

	if (!curfun)
		error(ESYNTAX);
	if (i >= NR_FUNPARAM)
		error(EPARNUM);
	sym = &tbl[i];
	sym->id = i;
	return sym;
}

static Symbol *
local(char *num)
{
	static Symbol tbl[NR_INT_IDENT];
	Symbol *sym;
	unsigned i = atoi(num);

	if (!curfun)
		error(ESYNTAX);
	if (i >= NR_INT_IDENT)
		error(EINTNUM);
	sym = &tbl[i];
	sym->id = i;
	return sym;
}

static Symbol *
global(char *num)
{
	static Symbol tbl[NR_EXT_IDENT];
	Symbol *sym;
	unsigned i = atoi(num);

	if (i >= NR_EXT_IDENT)
		error(EEXTNUM);

	sym = &tbl[i];
	sym->id = i;
	return sym;
}

static Node *
newnode(void)
{
	if (newp == &nodepool[NR_NODEPOOL])
		error(ENODEOV);
	return newp++;
}

Node *
imm(TINT i)
{
	Node *np = newnode();

	np->op = CONST;
	np->type = l_int16;
	/* FIX: memory leak */
	np->sym = xmalloc(sizeof(Symbol));
	np->sym->u.imm = i;
	np->left = np->right = NULL;
	return np;
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
	case L_FUNCTION:
		return &Funct;
	default:
		error(ETYPERR);
	}
}

static Symbol *
symbol(uint8_t t, char *token)
{
	Symbol *sym;
	static Symbol *(*tbl[3])(char *)= {
		[LOCAL] = local,
		[GLOBAL] = global,
		[PARAMETER] = parameter
	};
	sym = (*tbl[t])(token+1);
	sym->kind = *token;
	return sym;
}

static void
variable(uint8_t t, char *token)
{
	Node *np = newnode();
	Symbol *sym = symbol(t, token);

	np->sym = sym;
	np->op = sym->u.v.sclass;
	np->type = sym->u.v.type;
	np->left = np->right = NULL;
	push(np);
}

static void
localvar(char *token)
{
	variable(LOCAL, token);
}

static void
globvar(char *token)
{
	variable(GLOBAL, token);
}

static void
paramvar(char *token)
{
	variable(PARAMETER, token);
}

static void
immediate(char *token)
{
	/* TODO: check type of immediate */
	push(imm(atoi(token+2)));
}

static void
unary(char *token)
{
	Node *np = newnode();

	np->right = NULL;
	np->left = pop();
	np->type = *gettype(token+1);
	np->op = token[0];
	push(np);
}

static void
operator(char *token)
{
	Node *np = newnode();

	np->right = pop();
	np->left = pop();
	np->type = *gettype(token+1);
	np->op = token[0];
	push(np);
}

static void
label(char *token)
{
	Node *np = newnode();

	np->left = np->right = NULL;
	np->op = LABEL;
	np->sym = local(token);
	push(np);
}

static void
increment(char *token)
{
	Node *np = newnode();

	np->right = pop();
	np->left = pop();
	np->type = *gettype(token+2);
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
		np->type = *gettype(token);
		break;
	}
	push(np);
}

static void
cast(char *token)
{
	Node *np = newnode();

	np->right = NULL;
	np->left = pop();
	np->op = OCAST;
	np->type = *gettype(token+1);
	push(np);
}

static void
expr(char *token)
{
	void (*fun)(char *);
	unsigned c;

	do {
		if ((c = token[0]) > 0x7f || (fun = optbl[c]) == NULL)
			error(ESYNTAX);
		(*fun)(token);
	} while (token = strtok(NULL, "\t"));
}

static void
expression(char *token)
{
	Node *np;

	if (!curfun)
		error(ESYNTAX);

	expr(token);

	np = pop();
	if (stackp != stack)
		error(EEXPBAL);
	if (listp == &listexp[NR_EXPRESSIONS])
		error(EEXPROV);
	*listp++ = np;
}

static void
oreturn(char *token)
{
	Node *np = newnode(), *lp;

	np->op = token[0];

	if (token = strtok(NULL, "\t")) {
		expr(token);
		lp = pop();
		np->left = lp;
		np->type = lp->type;
	} else {
		np->left = NULL;
	}
	np->right = NULL;
	push(np);
}

static void
deflabel(char *token)
{
	Symbol *sym;

	if (!curfun)
		error(ESYNTAX);
	sym = local(token);
}

static Symbol *
declaration(uint8_t t, char class, char *token)
{
	Symbol *sym = symbol(t, token);
	char *s;

	free(sym->name);
	memset(sym, 0, sizeof(*sym));
	sym->u.v.sclass = class;

	if ((s = strtok(NULL, "\t")) == NULL)
		error(ESYNTAX);
	sym->u.v.type = *gettype(s);
	if ((s = strtok(NULL, "\t")) != NULL)
		sym->name = xstrdup(s);

	return sym;
}

static void
globdcl(char *token)
{
	Symbol *sym = declaration(GLOBAL, MEM, token);

	switch (token[0]) {
	case 'X':
		sym->extrn = 1;
		break;
	case 'G':
		sym->public = 1;
		break;
	}

	if (sym->u.v.type.letter != L_FUNCTION)
		return;

	if (curfun)
		error(ESYNTAX);

	curfun = sym;
	sym->u.f.body = listp = listexp;
	newp = nodepool;
}

static void
paramdcl(char *token)
{
	Symbol *sym = declaration(PARAMETER, AUTO, token);
	sym->u.v.off = -curfun->u.f.params;
	curfun->u.f.params += sym->u.v.type.size;
}

static void
localdcl(char *token)
{
	Symbol *sym = declaration(LOCAL, token[0], token);
	char sclass = *token;

	if (sclass == 'A' || sclass == 'R') {
		uint8_t size = sym->u.v.type.size;
		/* stack elements are 2 byte multiple */
		if (size == 1)
			++size;
		curfun->u.f.locals += size;
		sym->u.v.off = curfun->u.f.locals;
	}
}

void
parse(void)
{
	void (*fun)(char *tok);
	uint8_t len;
	int c;
	char line[MAXLINE];

	curfun = NULL;
	stackp = stack;
	listp = listexp;
	newp = nodepool;

	for (;;) {
		switch (c = getchar()) {
		case 'L':
			fun = deflabel;
			break;
		case '\t':
			fun = expression;
			break;
		case 'S':
			/* TODO: struct */
			break;
		case 'P':
			fun = paramdcl;
			break;
		case 'A': case 'R': case 'T':
			fun = localdcl;
			break;
		case 'Y': case 'G':
			fun = globdcl;
			break;
		case '}':
			if (curfun)
				return;
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

syntax_error:
	error(ESYNTAX);
}
