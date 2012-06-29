#include <stddef.h>
#include <stdio.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "syntax.h"
#include "code.h"

extern nodeop op_ary, op_call, op_field, op_ptr, op_postinc, op_postdec,
	op_preinc, op_predec, op_addr, op_indir, op_minus, op_plus, op_cpl,
	op_neg, op_mul, op_div, op_mod, op_add, op_sub, op_shl, op_shr,
	op_lt, op_gt, op_ge, op_le, op_eq, op_ne, op_band, op_bxor, op_bor,
	op_and, op_or, op_tern, op_assign, op_a_mul, op_a_div, op_a_mod,
	op_a_add, op_a_sub, op_a_shl, op_a_shr, op_a_and, op_a_xor, op_a_or;

struct node *expr(void);

static struct node *primary(void)
{
	register struct node *np;

	switch (yytoken) {
	case IDEN:
		if (!yyval.sym)
			error("'%s' undeclared", yytext);
	case CONSTANT:
		next();
		np = leaf(yyval.sym);
		break;
	case '(':
		next();
		np = expr();
		expect(')');
		break;
	default:
		error("unexpected element");
	}
	return np;
}

static struct node *postfix(void)
{
	register struct node *np1, *np2;

	np1 = primary();		/* TODO: fix ( case */
	for (;;) {
		register nodeop *op;
		switch (yytoken) {
		case '[':
			next();
			np2 = expr();
			expect(']');
			op = op_ary;
			goto node_2_childs; /* TODO: better names */
		case '(':
			next();
			np2 = expr();
			expect(')');
			op = op_call;
			goto node_2_childs;
		case '.':   op = op_field; goto expect_iden;
		case INDIR: op = op_ptr; goto expect_iden;
		case INC:   op = op_postinc; goto next;
		case DEC:   op = op_postdec; goto next;
		default:    return np1;
		}
	node_2_childs:
		np1 = op2(op, np1, np2);
		continue;
	expect_iden:
		next();
		expect(IDEN);
		np1 = op2(op, np1, leaf(yyval.sym));
		continue;
	next:
		np1 = op1(op, np1);
		next();
		continue;
	}
}

static struct node *cast(void);

static struct node *unary(void)
{
	register nodeop *op;

	switch (yytoken) {
	case SIZEOF:		/* TODO: Implement sizeof */
		next();
		if (accept('(')) {
			type_name();
			expect(')');
		} else {
			unary();
		}
		return leaf(NULL);
	case INC: op = op_preinc; goto call_unary;
	case DEC: op = op_predec; goto call_unary;
	case '&': op = op_addr;  goto call_cast;
	case '*': op = op_indir; goto call_cast;
	case '-': op = op_minus; goto call_cast;
	case '+': op = op_plus; goto call_cast;
	case '~': op = op_cpl; goto call_cast;
	case '!': op = op_neg; goto call_cast;
	default: return postfix();
	}

call_cast:
	next();
	return op1(op, cast());

call_unary:
	next();
	return op1(op, unary());
}

static struct node *cast(void)
{
	while (accept('(')) {	/* TODO: Implement casts */
		type_name();	/* check if it really is a type name */
		expect(')');
	}
	return unary();
}

static struct node *mul(void)
{
	register struct node *np;
	register nodeop *op;

	np = cast();
	for (;;) {
		switch (yytoken) {
		case '*': op = op_mul; break;
		case '/': op = op_div; break;
		case '%': op = op_mod; break;
		default:  return np;
		}
		next();
		np = op2(op, np, cast());
	}
}

static struct node *add(void)
{
	register nodeop *op;
	register struct node *np;

	np = mul();
	for (;;) {
		switch (yytoken) {
		case '+': op = op_add; break;
		case '-': op = op_sub; break;
		default:  return np;
		}
		next();
		np = op2(op, np, mul());
	}
}

static struct node *shift(void)
{
	register nodeop *op;
	register struct node *np;

	np = add();
	for (;;) {
		switch (yytoken) {
		case SHL: op = op_shl; break;
		case SHR: op = op_shr; break;
		default:  return np;
		}
		next();
		np = op2(op, np, add());
	}
}

static struct node *relational(void)
{
	register nodeop *op;
	register struct node *np;

	np = shift();
	for (;;) {
		switch (yytoken) {
		case '<': op = op_lt; break;
		case '>': op = op_gt; break;
		case GE:  op = op_ge; break;
		case LE:  op = op_le; break;
		default:  return np;
		}
		next();
		np = op2(op, np, shift());
	}
}

static struct node *eq(void)
{
	register nodeop *op;
	register struct node *np;

	np = relational();
	for (;;) {
		switch (yytoken) {
		case EQ: op = op_eq; break;
		case NE: op = op_ne; break;
		default: return np;
		}
		next();
		np = op2(op, np, relational());
	}
}

static struct node *bit_and(void)
{
	register struct node *np;

	np = eq();
	while (yytoken == '&') {
		next();
		np = op2(op_band, np, eq());
	}
	return np;
}

static struct node *bit_xor(void)
{
	register struct node *np;

	np = bit_and();
	while (yytoken == '^') {
		next();
		np = op2(op_bxor, np, bit_and());
	}
	return np;
}

static struct node *bit_or(void)
{
	register struct node *np;

	np = bit_xor();
	while (yytoken == '|') {
		next();
		np = op2(op_bor, np, bit_xor());
	}
	return np;
}

static struct node *and(void)
{
	register struct node *np;

	np = bit_or();
	while (yytoken == AND) {
		next();
		np = op2(op_and, np, bit_or());
	}
	return np;
}

static struct node *or(void)
{
	register struct node *np;

	np = and();
	while (yytoken == OR) {
		next();
		np = op2(op_or, np, and());
	}
	return np;
}

static struct node *cond(void)
{
	register struct node *np, *aux;

	np = or();
	while (yytoken == '?') {
		aux = expr();
		expect(':');
		np = op3(op_tern, np, aux, or());
	}
	return np;
}

static struct node *assign(void)
{
	register nodeop *op;
	register struct node *np;

	np = cond();
	for (;;) {
		switch (yytoken) {
		case '=': op = op_assign; break;
		case MUL_EQ: op = op_a_mul; break;
		case DIV_EQ: op = op_a_div; break;
		case MOD_EQ: op = op_a_mod; break;
		case ADD_EQ: op = op_a_add; break;
		case SUB_EQ: op = op_a_sub; break;
		case SHL_EQ: op = op_a_shl; break;
		case SHR_EQ: op = op_a_shr; break;
		case AND_EQ: op = op_a_and; break;
		case XOR_EQ: op = op_a_xor; break;
		case OR_EQ: op = op_a_or; break;
		default: return np;
		}
		next();
		np = op2(op, np, assign());
	}
	return np;
}

struct node *expr(void)
{
	register struct node *np;

	do
		np = assign();
	while (yytoken == ',');

	return np;
}
