/*
name: TEST009
description: Basic test for loops
output:
F1
G1	F1	main
{
-
A2	I	x
	A2	#I0	:I
	j	L5
	d
L3
	A2	A2	#I1	+I	:I
L5
	j	L3	A2	#IA	<I
	b
L4
	j	L6	A2	#IA	=I
	y	#I1
L6
	y	#I0
}
*/

int 
main()
{
	int x;
	
	for (x = 0; x < 10 ; x = x + 1)
		;
	if (x != 10)
		return 1;
	return 0;
}

