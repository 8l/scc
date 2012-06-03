
#include <stddef.h>

#include "tokens.h"
#include "syntax.h"


void stmt(void);

static void do_goto(void)
{
	puts("void do_goto");
	expect(GOTO);
	expect(IDENTIFIER);
	puts("leaving void do_goto");
}

static void do_while(void)
{
	puts("void do_while");
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
	stmt();
	puts("leaving void do_while");
}

static void do_do(void)
{
	puts("void do_do");
	expect(DO);
	stmt();
	expect(WHILE);
	expect('(');
	expr();
	expect(')');
	puts("leaving void do_do");
}

static void do_for(void)
{
	puts("void do_for");
	expect(FOR);
	expect('(');
	if (yytoken != ';')
		expr();
	expect(';');
	if (yytoken != ';')
		expr();
	expect(';');
	if (yytoken != ')')
		expr();
	expect(')');
	stmt();
	puts("leaving void do_for");
}

static void do_if(void)
{
	puts("void do_if");
	expect(IF);
	expect('(');
	expr();
	expect(')');
	stmt();
	if (accept(ELSE))
		stmt();

	puts("leaving void do_if");
}

static void do_switch(void)
{
	puts("do_switch");
	expect(SWITCH);
	expect('(');
	expr();
	expect(')');
	stmt();
	puts("leaving do_switch");
}

void stmt(void)
{
	puts("stmt");
	unsigned char tok;

	switch (yytoken) {
	case '{':
		compound();
		break;
	case SWITCH:
		do_switch();
		break;
	case IF:
		do_if();
		break;
	case FOR:
		do_for();
		break;
	case DO:
		do_do();
		break;
	case WHILE:
		do_while();
		break;
	case CONTINUE:
	case BREAK:
	case RETURN:
	case GOTO:
		do_goto();
		break;
	case CASE:
		/* TODO */
	case DEFAULT:
		/* TODO */
		break;
	case IDENTIFIER:
		/* TODO: check if it can be a label */
	default:
		expr();
		expect(';');
		break;
	}
	puts("leaving stmt");
}

void compound(void)
{
	puts("compound");
	if (accept('{')) {
		decl_list();
		while (!accept('}'))
			stmt();
	}
	puts("leaving compound");
}
