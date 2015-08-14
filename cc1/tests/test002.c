/*
name: TEST002
description: Test forward references before definition of types
output:
S2	S
G4	P	x
F1
X6	F1	main
G6	F1	main	{
-
S2	S	(
M5	I
)
A2	S2	y
	A2	M5	.I	#I0	:I
	G4	@S2	A2	:S
}
*/

struct S *x;
struct S {
	int i;
};

void
main(void)
{
	struct S y;

	y.i = 0;
	*x = y;
}
