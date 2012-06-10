#include <stddef.h>

#include "tokens.h"
#include "symbol.h"
#include "syntax.h"

#include <stdio.h>		/* TODO: remove this */


void expr(void);

static void primary(void)
{
	switch (yytoken) {
	case IDEN:
		if (!yyval.sym)
			error("'%s' undeclared", yytext);
	case CONSTANT:
		next();
		break;
	case '(':
		next();
		expr();
		expect(')');
		break;
	}
}

static void postfix(void)
{
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
		case PTR:
			next();
			expect(IDEN);
			break;
		case INC:
		case DEC:
			next();
			break;
		default:
			return;
		}
	}
}

static void cast(void);

static void unary(void)
{
	for (;;) {
		switch (yytoken) {
		case SIZEOF:
			next();
			if (accept('(')) {
				type_name();
				expect(')');
				return;
			}
			break;
		case INC:
		case DEC:
			next();
			break;
		case '&': case '*': case '-': case '~': case '!': case '+':
			next();
			cast();
			return;
		default:
			postfix();
			return;
		}
	}
}

static void cast(void)
{
	while (accept('(')) {
		type_name();	/* check if it really is a type name */
		expect(')');
	}
	unary();
}

static void mul(void)
{
	do
		cast();
	while(accept('*') || accept('/') || accept('%'));
}

static void add(void)
{
	do
		mul();
	while (accept('+') || accept('-'));
}

static void shift(void)
{
	do
		add();
	while (accept(LSHIFT) || accept(RSHIFT));
}

static void relational(void)
{
	do
		shift();
	while (accept('<') || accept('>') || accept(GE) || accept(LE));
}

static void equality(void)
{
	do
		relational();
	while (accept(EQ) || accept(NE));
}

static void bit_and(void)
{
	do
		equality();
	while (accept('&'));
}

static void bit_exor(void)
{
	do
		bit_and();
	while (accept('^'));
}

static void bit_or(void)
{
	do
		bit_exor();
	while (accept('|'));
}

static void and(void)
{
	do
		bit_or();
	while (accept(AND));
}

static void or(void)
{
	do
		and();
	while (accept(OR));
}

static void conditional(void)
{
	or();
	if (accept('?')) {
		expr();
		expect(':');
		conditional();
	}
}

static void assign(void)
{
	unary();
	switch (yytoken) {
	case '=':
	case MUL_EQ:
	case DIV_EQ:
	case MOD_EQ:
	case ADD_EQ:
	case SUB_EQ:
	case LSHIFT_EQ:
	case RSHIFT_EQ:
	case AND_EQ:
	case XOR_EQ:
	case OR_EQ:
		next();
		assign();
		break;
	default:
		conditional();
		break;
	}
}

void expr(void)
{
	do
		assign();
	while (yytoken == ',');
}
