/*
name: TEST009
description: Basic test for loops
output:
F1
G2	F1	main
{
\
A3	I	x
	A3	#I0	:I
	j	L6
	e
L4
	A3	A3	#I1	+I	:I
L6
	j	L4	A3	#IA	<I
	b
L5
	j	L7	A3	#IA	=I
	r	#I1
L7
	r	#I0
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

