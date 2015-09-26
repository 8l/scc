
/*
name: TEST028
description: Test of reinterpretation in define
output:
F5
G6	F5	foo
{
\
	r	"6869	'P
}
*/


#define M(x) x
#define A(a,b) a(b)

char *
foo(void)
{
	return A(M,"hi");
}
