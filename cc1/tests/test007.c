/*
name: TEST007
description: basic while test
output:
F1
G1	F1	main
{
-
A2	I	x
	A2	#IA	:I
	j	L5
	d
L3
	A2	A2	#I1	-I	:I
L5
	j	L3	A2	#I0	!I
	b
L4
	y	A2
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
