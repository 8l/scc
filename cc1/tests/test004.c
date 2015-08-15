/*
name: TEST004
description: Test integer operations
output:
F1
G1	F1	main
{
-
A2	I	x
	A2	#I0	:I
	A2	A2	#I2	+I	:I
	A2	A2	#I1	-I	:I
	A2	A2	#I6	*I	:I
	A2	A2	#I2	/I	:I
	A2	A2	#I2	%I	:I
	A2	A2	#I2	lI	:I
	A2	A2	#I1	rI	:I
	A2	A2	#IFF	|I	:I
	A2	A2	#I3	&I	:I
	A2	A2	#I1	^I	:I
	A2	A2	A2	#I1	>I	#I1	#I0	?I	+I	:I
	A2	A2	A2	#I3	<I	#I1	#I0	?I	+I	:I
	A2	A2	A2	#I1	>I	#I1	#I0	?I	+I	:I
	A2	A2	A2	#I4	<I	#I1	#I0	?I	+I	:I
	j	L3	A2	#I4	=I
	yI	#I1
L3
	yI	#I0
}
*/

int
main()
{
	int x;

	x = 0;
	x = x + 2;        // 2
	x = x - 1;        // 1
	x = x * 6;        // 6
	x = x / 2;        // 3
	x = x % 2;        // 1
	x = x << 2;       // 4
	x = x >> 1;       // 2
	x = x | 255;      // 255
	x = x & 3;        // 3
	x = x ^ 1;        // 2
	x = x + (x > 1);  // 2
	x = x + (x < 3);  // 2
	x = x + (x > 1);  // 3
	x = x + (x < 4);  // 4
	if(x != 4)
		return 1;
	return 0;
}
