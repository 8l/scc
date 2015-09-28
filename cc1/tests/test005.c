/*
name: TEST005
description: Test unary integer operations
output:
F1	I	E
G2	F1	main
{
\
A3	I	x
	A3	#I3	:I
	A3	A3	#I0	=I	:I
	A3	A3	#I0	=I	:I
	A3	A3	~I	:I
	A3	A3	_I	:I
	j	L4	A3	#I2	=I
	r	#I1
L4
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
