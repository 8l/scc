#include <stddef.h>

#include "tokens.h"
#include "symbol.h"
#include "syntax.h"

#include <stdio.h>		/* TODO: remove this */


void expr(void);

static void primary(void)
{
	puts("primary");
	switch (yytoken) {
	case IDENTIFIER:
		if (!yyval.sym)
			error("'%s' undeclared", yytext);
	case CONSTANT:
	case STRING_LITERAL:
		next();
		break;
	case '(':
		next();
		expr();
		expect(')');
		break;
	}
	puts("leaving primary");
}

static void postfix(void)
{
	puts("postfix");
	primary();
	for (;;) {
		switch (yytoken) {
		case '[':
			next();
			expr();
			expect(']');
			break;
		case '(':
			next();
			expr();
			expect(')');
			break;
		case '.':
		case PTR_OP:
			next();
			expect(IDENTIFIER);
			break;
		case INC_OP:
		case DEC_OP:
			next();
			break;
		default:
			puts("leaving postfix");
			return;
		}
	}
}

static void cast(void);

static void unary(void)
{
	puts("unary");
	for (;;) {
		switch (yytoken) {
		case SIZEOF:
			next();
			if (accept('(')) {
				type_name();
				expect(')');
				goto leaving;
			}
			break;
		case INC_OP:
		case DEC_OP:
			next();
			break;
		case '&': case '*': case '-': case '~': case '!': case '+':
			next();
			cast();
			goto leaving;
		default:
			postfix();
			goto leaving;
		}
	}
leaving:
	puts("leaving unary");
}

static void cast(void)
{
	puts("cast");
	while (accept('(')) {
		type_name();	/* check if it really is a type name */
		expect(')');
	}
	unary();
	puts("leaving cast");
}

static void mul(void)
{
	puts("mul");
	do
		cast();
	while(accept('*') || accept('/') || accept('%'));
	puts("leaving mul");
}

static void add(void)
{
	puts("add");
	do
		mul();
	while (accept('+') || accept('-'));
	puts("leaving add");
}

static void shift(void)
{
	puts("shift");
	do
		add();
	while (accept(LSHIFT_OP) || accept(RSHIFT_OP));
	puts("leaving shift");
}

static void relational(void)
{
	puts("relational");
	do
		shift();
	while (accept('<') || accept('>') || accept(GE_OP) || accept(LE_OP));
	puts("leaving relational");
}

static void equality(void)
{
	puts("equality");
	do
		relational();
	while (accept(EQ_OP) || accept(NE_OP));
	puts("leaving equality");
}

static void bit_and(void)
{
	puts("bit_and");
	do
		equality();
	while (accept('&'));
	puts("leaving bit_and");
}

static void bit_exor(void)
{
	puts("bit_exor");
	do
		bit_and();
	while (accept('^'));
	puts("leaving bit_exor");
}

static void bit_or(void)
{
	puts("bit_or");
	do
		bit_exor();
	while (accept('|'));
	puts("leaving bit_or");
}

static void and(void)
{
	puts("and");
	do
		bit_or();
	while (accept(AND_OP));
	puts("leaving and");
}

static void or(void)
{
	puts("or");
	do
		and();
	while (accept(OR_OP));
	puts("leaving or");
}

static void conditional(void)
{
	puts("conditional");
	or();
	if (accept('?')) {
		expr();
		expect(':');
		conditional();
	}
	puts("leaving conditional");
}

static void assign(void)
{
	puts("assign");
	unary();
	switch (yytoken) {
	case '=':
	case MUL_ASSIGN:
	case DIV_ASSIGN:
	case MOD_ASSIGN:
	case ADD_ASSIGN:
	case SUB_ASSIGN:
	case LSHIFT_ASSIGN:
	case RSHIFT_ASSIGN:
	case AND_ASSIGN:
	case XOR_ASSIGN:
	case OR_ASSIGN:
		next();
		assign();
		break;
	default:
		conditional();
		break;
	}
	puts("leaving assign");
}

void expr(void)
{
	puts("expr");
	do
		assign();
	while (yytoken == ',');
	puts("leaving expr");
}
