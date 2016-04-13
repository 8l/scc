/*
name: TEST002
description: Test forward references before definition of types
error:
output:
G4	P	"x
G8	F	"main
{
\
S2	"S	#NC	#N2
M5	I	"i	#N0
M6	P	"next	#N2
A9	S2	"y
A10	P	"p
A11	N	"n
	A9	M5	.I	#I0	:I
	G4	@S2	A9	:S2
	A11	#N0	:N	A10	A9	'P	:P	,P
	j	L14
	e
L12
	A11	#N1	:+N	A10	A10	@S2	M6	.P	:P	,P
L14
	y	L12	A10	#P0	!I
	b
L13
}
*/

struct S *x;
struct S {
	int i;
	struct S *next;
};

int
main(void)
{
	struct S y, *p;
	unsigned n;

	y.i = 0;
	*x = y;

	for (n = 0, p = &y; p; ++n, p = p->next)
		/* nothing */;
}
