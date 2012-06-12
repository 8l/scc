#include <stddef.h>
#include <stdio.h>

#include "cc.h"
#include "tokens.h"
#include "symbol.h"
#include "syntax.h"
#include "code.h"

extern void gen_ary(void), gen_call(void), gen_or(void), gen_tern(void),
	gen_band(void), gen_bxor(void), gen_bor(void), gen_and(void),
	gen_sizeof(void), gen_field(void), gen_ptr(void), gen_preinc(void),
	gen_predec(void), gen_addr(void), gen_indir(void), gen_minus(void),
	gen_plus(void), gen_cpl(void), gen_neg(void), gen_mul(void),
	gen_div(void), gen_mod(void), gen_add(void), gen_sub(void),
	gen_shl(void), gen_shr(void), gen_lt(void), gen_gt(void),
	gen_ge(void), gen_le(void), gen_eq(void), gen_ne(void),
	gen_a_mul(void), gen_a_div(void), gen_a_mod(void), gen_a_add(void),
	gen_a_sub(void), gen_a_shl(void), gen_a_shr(void), gen_a_and(void),
	gen_a_xor(void), gen_a_or(void), gen_postinc(void),
	gen_postdec(void), gen_assign(void);


void expr(void);

static void primary(void)
{
	fputs("static void primary(void)", stderr);
	switch (yytoken) {
	case IDEN:
		if (!yyval.sym)
			error("'%s' undeclared", yytext);
	case CONSTANT:
		next();
		push(yyval.sym);
		break;
	case '(':
		next();
		expr();
		expect(')');
		break;
	}
	fputs("leaving static void primary(void)", stderr);
}

static void postfix(void)
{
	fputs("static void postfix(void)", stderr);
	primary();		/* TODO: fix ( case */
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case '[':
			next();
			expr();
			expect(']');
			gen_ary();
			continue;
		case '(':
			next();
			expr();
			expect(')');
			gen_call();
			continue;
		case '.': fp = gen_field; goto expect_iden;
		case PTR: fp = gen_ptr; goto expect_iden;
		case INC: fp = gen_postinc; goto next;
		case DEC: fp = gen_postdec; goto next;
		default:
			fputs("leaving static void postfix(void)", stderr);
			return;
		}
	expect_iden:
		next();
		expect(IDEN);
		fp();
		continue;
	next:
		next();
		continue;
	}
}

static void cast(void);

static void unary(void)
{
	fputs("static void unary(void)", stderr);
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case SIZEOF:
			next();
			if (accept('(')) {
				type_name();
				expect(')');
			} else {
				unary();
			}
			gen_sizeof();
			continue;
		case INC: fp = gen_preinc; goto call_unary;
		case DEC: fp = gen_predec; goto call_unary;
		case '&': fp = gen_addr;  goto call_cast;
		case '*': fp = gen_indir; goto call_cast;
		case '-': fp = gen_minus; goto call_cast;
		case '+': fp = gen_plus; goto call_cast;
		case '~': fp = gen_cpl; goto call_cast;
		case '!': fp = gen_neg; goto call_cast;
		default:
			postfix();
			fputs("leaving static void unary(void)", stderr);
			return;
		}
	call_cast:
		next();
		cast();
		fp();
		return;
	call_unary:
		next();
		unary();
		fp();
		return;
	}
}

static void cast(void)
{
	fputs("static void cast(void)", stderr);
	while (accept('(')) {
		type_name();	/* check if it really is a type name */
		expect(')');
	}
	unary();
	fputs("leaving static void cast(void)", stderr);
}

static void mul(void)
{
	fputs("static void mul(void)", stderr);
	cast();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case '*': fp = gen_mul; break;
		case '/': fp = gen_div; break;
		case '%': fp = gen_mod; break;
		default:
			fputs("leaving static void mul(void)", stderr);
			return;
		}
		next();
		cast();
		fp();
	}
}

static void add(void)
{
	fputs("static void add(void)", stderr);
	mul();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case '+': fp = gen_add; break;
		case '-': fp = gen_sub; break;
		default:
			fputs("leaving static void add (void)", stderr);
			return;
		}
		next();
		mul();
		fp();
	}
}

static void shift(void)
{
	fputs("static void shift(void)", stderr);
	add();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case SHL: fp = gen_shl; break;
		case SHR: fp = gen_shr; break;
		default:
			fputs("leaving static void shift (void)", stderr);
			return;
		}
		next();
		add();
		fp();
	}
}

static void relational(void)
{
	fputs("static void relational(void)", stderr);
	shift();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case '<': fp = gen_lt; break;
		case '>': fp = gen_gt; break;
		case GE: fp = gen_ge; break;
		case LE: fp = gen_le; break;
		default:
			fputs("leaving static void relational (void)", stderr);
			return;
		}
		next();
		shift();
		fp();
	}
}

static void eq(void)
{
	fputs("static void eq(void)", stderr);
	relational();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case EQ: fp = gen_eq; break;
		case NE: fp = gen_ne; break;
		default:
			fputs("leaving static void eq (void)", stderr);
			return;
		}
		next();
		relational();
		fp();
	}
}

static void bit_and(void)
{
	fputs("static void bit_and(void)", stderr);
	eq();
	while (yytoken == '&') {
		next();
		eq();
		gen_band();
	}
	fputs("leaving static void bit_and (void)", stderr);
}

static void bit_xor(void)
{
	fputs("static void bit_xor(void)", stderr);
	bit_and();
	while (yytoken == '^') {
		next();
		bit_and();
		gen_bxor();
	}
	fputs("leaving static void bit_xor(void)", stderr);
}

static void bit_or(void)
{
	fputs("static void bit_or(void)", stderr);
	bit_xor();
	while (yytoken == '|') {
		next();
		bit_xor();
		gen_bor();
	}
	fputs("leaving static void bit_or(void)", stderr);
}

static void and(void)
{
	fputs("static void and(void)", stderr);
	bit_or();
	while (yytoken == AND) {
		next();
		bit_or();
		gen_and();
	}
	fputs("leaving static void and(void)", stderr);
}

static void or(void)
{
	fputs("static void or(void)", stderr);
	and();
	while (yytoken == OR) {
		next();
		and();
		gen_or();
	}
	fputs("leaving static void or(void)", stderr);
}

static void cond(void)
{
	fputs("static void cond(void)", stderr);
	or();
	while (yytoken == '?') {
			expr();
			expect(':');
			or();
			gen_tern();
	}
	fputs("leaving static void cond(void)", stderr);
}

static void assign(void)
{
	fputs("static void assign(void)", stderr);
	cond();
	for (;;) {
		register void (*fp)(void);
		switch (yytoken) {
		case '=': fp = gen_assign; break;
		case MUL_EQ: fp = gen_a_mul; break;
		case DIV_EQ: fp = gen_a_div; break;
		case MOD_EQ: fp = gen_a_mod; break;
		case ADD_EQ: fp = gen_a_add; break;
		case SUB_EQ: fp = gen_a_sub; break;
		case SHL_EQ: fp = gen_a_shl; break;
		case SHR_EQ: fp = gen_a_shr; break;
		case AND_EQ: fp = gen_a_and; break;
		case XOR_EQ: fp = gen_a_xor; break;
		case OR_EQ: fp = gen_a_or; break;
		default:
			fputs("static void assign (void)", stderr);
			return;
		}
		next();
		assign();
		fp();
	}
}

void expr(void)
{
	fputs("void expr(void)", stderr);
	do
		assign();
	while (yytoken == ',');
}
