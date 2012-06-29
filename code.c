#include <stddef.h>

#include "cc.h"
#include "code.h"
#include "symbol.h"


#define NR_MACHINE_OP 30
#define NR_SYM_STACK 30

static struct symbol *stack[NR_SYM_STACK];
static struct symbol **stackp = stack;

void push(register struct symbol *sym)
{
	if (stackp == &stack[NR_SYM_STACK])
		error("out of memory");
	*stackp++ = sym;
}

void code(void *data)
{
}

void execute(void)
{
}



void gen_ary(void)
{
	puts("gen_ary");
}

void gen_call(void)
{
	puts("gen_call");
}

void gen_or(void)
{
	puts("gen_or");
}

void gen_tern(void)
{
	puts("gen_tern");
}

void gen_band(void)
{
	puts("gen_band");
}

void gen_bxor(void)
{
	puts("gen_bxor");
}

void gen_bor(void)
{
	puts("gen_bor");
}

void gen_and(void)
{
	puts("gen_and");
}

void gen_sizeof(void)
{
	puts("gen_sizeof");
}

void gen_field(void)
{
	puts("gen_field");
}

void gen_ptr(void)
{
	puts("gen_ptr");
}

void gen_preinc(void)
{
	puts("gen_preinc");
}

void gen_predec(void)
{
	puts("gen_predec");
}

void gen_addr(void)
{
	puts("gen_addr");
}

void gen_indir(void)
{
	puts("gen_indir");
}

void gen_minus(void)
{
	puts("gen_minus");
}

void gen_plus(void)
{
	puts("gen_plus");
}

void gen_cpl(void)
{
	puts("gen_cpl");
}

void gen_neg(void)
{
	puts("gen_neg");
}

void gen_mul(void)
{
	puts("gen_mul");
}

void gen_div(void)
{
	puts("gen_div");
}

void gen_mod(void)
{
	puts("gen_mod");
}

void gen_add(void)
{
	puts("gen_add");
}

void gen_sub(void)
{
	puts("gen_sub");
}

void gen_shl(void)
{
	puts("gen_shl");
}

void gen_shr(void)
{
	puts("gen_shr");
}

void gen_lt(void)
{
	puts("gen_lt");
}

void gen_gt(void)
{
	puts("gen_gt");
}

void gen_ge(void)
{
	puts("gen_ge");
}

void gen_le(void)
{
	puts("gen_le");
}

void gen_eq(void)
{
	puts("gen_eq");
}

void gen_ne(void)
{
	puts("gen_ne");
}

void gen_a_mul(void)
{
	puts("gen_a");
}

void gen_a_div(void)
{
	puts("gen_a");
}

void gen_a_mod(void)
{
	puts("gen_a");
}

void gen_a_add(void)
{
	puts("gen_a");
}

void gen_a_sub(void)
{
	puts("gen_a");
}

void gen_a_shl(void)
{
	puts("gen_a");
}

void gen_a_shr(void)
{
	puts("gen_a");
}

void gen_a_and(void)
{
	puts("gen_a");
}

void gen_a_xor(void)
{
	puts("gen_a");
}

void gen_a_or(void)
{
	puts("gen_a");
}

void gen_postinc(void)
{
	puts("gen_postinc");
}

void gen_postdec(void)
{
	puts("gen_postdec");
}

void gen_assign(void)
{
	puts("gen_assign");
}
