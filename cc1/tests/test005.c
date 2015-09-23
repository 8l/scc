/*
name: TEST005
description: Test unary integer operations
output:
F1
G1	F1	main
{
-
A2	I	x
	A2	#I3	:I
	A2	A2	#I0	=I	:I
	A2	A2	#I0	=I	:I
	A2	A2	~I	:I
	A2	A2	_I	:I
	j	L3	A2	#I2	=I
	r	#I1
L3
	r	#I0
}
*/


int
main()
{
	int x;

	x = 3;
	x = !x; //  0
	x = !x; //  1
	x = ~x; // -1
	x = -x; //  2
	if(x != 2)
		return 1;
	return 0;
}
