/*
name: TEST007
description: basic while test
output:
F1
G2	F1	main
{
\
A3	I	x
	A3	#IA	:I
	j	L6
	e
L4
	A3	A3	#I1	-I	:I
L6
	j	L4	A3	#I0	!I
	b
L5
	r	A3
}
*/

int
main()
{
	int x;
	
	x = 10;
	while (x)
		x = x - 1;
	return x;
}
