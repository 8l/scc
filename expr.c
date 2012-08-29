#include <stddef.h>
#include <stdio.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "syntax.h"
#include "code.h"


struct node *expr(void);

static struct node *
primary(void)
{
	register struct node *np;

	switch (yytoken) {
	case IDEN:
		if (!yyval.sym)
			error("'%s' undeclared", yytext);
	case CONSTANT:
		next();
		np = nodesym(yyval.sym);
		break;
	case '(':
		next();
		np = expr();
		expect(')');
		break;
	default:
		np = NULL;
		break;
	}
	return np;
}

static struct node *
postfix(void)
{
	register struct node *np1, *np2;

	np1 = primary();		/* TODO: fix ( case */
	for (;;) {
		register unsigned char op;
		switch (yytoken) {
		case '[':
			next();
			np2 = expr();
			expect(']');
			op = OARY;
			goto node_2_childs; /* TODO: better names */
		case '(':
			next();
			np2 = expr();
			expect(')');
			op = OCALL;
			goto node_2_childs;
		case '.':   op = OFIELD; goto expect_iden;
		case INDIR: op = OPTR; goto expect_iden;
		case INC:   op = OPOSTINC; goto next;
		case DEC:   op = OPOSTDEC; goto next;
		default:    return np1;
		}
	node_2_childs:
		np1 = node2(op, np1, np2);
		continue;
	expect_iden:
		next();
		expect(IDEN);
		np1 = node2(op, np1, nodesym(yyval.sym));
		continue;
	next:
		np1 = node1(op, np1);
		next();
		continue;
	}
}

static struct node *cast(void);

static struct node *
unary(void)
{
	register unsigned char op;

	switch (yytoken) {
	case SIZEOF:		/* TODO: Implement sizeof */
		next();
		if (accept('(')) {
			type_name();
			expect(')');
		} else {
			unary();
		}
		return nodesym(NULL);
	case INC: op = OPREINC; goto call_unary;
	case DEC: op = OPREDEC; goto call_unary;
	case '&': op = OADDR;  goto call_cast;
	case '*': op = OINDIR; goto call_cast;
	case '-': op = OMINUS; goto call_cast;
	case '+': op = OPLUS; goto call_cast;
	case '~': op = OCPL; goto call_cast;
	case '!': op = ONEG; goto call_cast;
	default: return postfix();
	}

call_cast:
	next();
	return node1(op, cast());

call_unary:
	next();
	return node1(op, unary());
}

static struct node *
cast(void)
{
	while (accept('(')) {	/* TODO: Implement casts */
		type_name();	/* check if it really is a type name */
		expect(')');
	}
	return unary();
}

static struct node *
mul(void)
{
	register struct node *np;
	register unsigned char op;

	np = cast();
	for (;;) {
		switch (yytoken) {
		case '*': op = OMUL; break;
		case '/': op = ODIV; break;
		case '%': op = OMOD; break;
		default:  return np;
		}
		next();
		np = node2(op, np, cast());
	}
}

static struct node *
add(void)
{
	register unsigned char op;
	register struct node *np;

	np = mul();
	for (;;) {
		switch (yytoken) {
		case '+': op = OADD; break;
		case '-': op = OSUB; break;
		default:  return np;
		}
		next();
		np = node2(op, np, mul());
	}
}

static struct node *
shift(void)
{
	register unsigned char op;
	register struct node *np;

	np = add();
	for (;;) {
		switch (yytoken) {
		case SHL: op = OSHL; break;
		case SHR: op = OSHR; break;
		default:  return np;
		}
		next();
		np = node2(op, np, add());
	}
}

static struct node *
relational(void)
{
	register unsigned char op;
	register struct node *np;

	np = shift();
	for (;;) {
		switch (yytoken) {
		case '<': op = OLT; break;
		case '>': op = OGT; break;
		case GE:  op = OGE; break;
		case LE:  op = OLE; break;
		default:  return np;
		}
		next();
		np = node2(op, np, shift());
	}
}

static struct node *
eq(void)
{
	register unsigned char op;
	register struct node *np;

	np = relational();
	for (;;) {
		switch (yytoken) {
		case EQ: op = OEQ; break;
		case NE: op = ONE; break;
		default: return np;
		}
		next();
		np = node2(op, np, relational());
	}
}

static struct node *
bit_and(void)
{
	register struct node *np;

	np = eq();
	while (yytoken == '&') {
		next();
		np = node2(OBAND, np, eq());
	}
	return np;
}

static struct node *
bit_xor(void)
{
	register struct node *np;

	np = bit_and();
	while (yytoken == '^') {
		next();
		np = node2(OBXOR, np, bit_and());
	}
	return np;
}

static struct node *
bit_or(void)
{
	register struct node *np;

	np = bit_xor();
	while (yytoken == '|') {
		next();
		np = node2(OBOR, np, bit_xor());
	}
	return np;
}

static struct node *
and(void)
{
	register struct node *np;

	np = bit_or();
	while (yytoken == AND) {
		next();
		np = node2(OAND, np, bit_or());
	}
	return np;
}

static struct node *
or(void)
{
	register struct node *np;

	np = and();
	while (yytoken == OR) {
		next();
		np = node2(OOR, np, and());
	}
	return np;
}

static struct node *
cond(void)
{
	register struct node *np, *aux;

	np = or();
	while (yytoken == '?') {
		aux = expr();
		expect(':');
		np = node3(OTERN, np, aux, or());
	}
	return np;
}

static struct node *
assign(void)
{
	register struct node *np = cond();

	for (;;) {
		register unsigned char op;

		switch (yytoken) {
		case '=':    op = OASSIGN; break;
		case MUL_EQ: op = OA_MUL;  break;
		case DIV_EQ: op = OA_DIV;  break;
		case MOD_EQ: op = OA_MOD;  break;
		case ADD_EQ: op = OA_ADD;  break;
		case SUB_EQ: op = OA_SUB;  break;
		case SHL_EQ: op = OA_SHL;  break;
		case SHR_EQ: op = OA_SHR;  break;
		case AND_EQ: op = OA_AND;  break;
		case XOR_EQ: op = OA_XOR;  break;
		case OR_EQ:  op = OA_OR;   break;
		default:  goto return_np;
		}
		next();
		np = node2(op, np, assign());
	}
return_np:
	return np;
}

struct node *
expr(void)
{
	register struct node *np;

	do
		np = assign();
	while (yytoken == ',');

	return np;
}
