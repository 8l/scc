/*
name: TEST002
description: Test forward references before definition of types
output:
G4	P	x
F1
G7	F1	main
{
-
S2	S
M5	I	i
M6	P	next
A2	S2	y
A3	P	p
A4	N	n
	A2	M5	.I	#I0	:I
	G4	@S2	A2	:S2
	A4	#N0	:N	A3	A2	'P	:P	,P
	j	L7
	d
L5
	A4	#N1	:+N	A3	A3	@S2	M6	.P	:P	,P
L7
	j	L5	A3	#P0	!I
	b
L6
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
