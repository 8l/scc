/* See LICENSE file for copyright and license details. */

/*
name: TEST004
description: Test integer operations
error:
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#I0	:I
	A3	A3	#I2	+I	:I
	A3	A3	#I1	-I	:I
	A3	A3	#I6	*I	:I
	A3	A3	#I2	/I	:I
	A3	A3	#I2	%I	:I
	A3	A3	#I2	lI	:I
	A3	A3	#I1	rI	:I
	A3	A3	#IFF	|I	:I
	A3	A3	#I3	&I	:I
	A3	A3	#I1	^I	:I
	A3	A3	A3	#I1	>I	+I	:I
	A3	A3	A3	#I3	<I	+I	:I
	A3	A3	A3	#I1	>I	+I	:I
	A3	A3	A3	#I4	<I	+I	:I
	y	L4	A3	#I4	=I
	h	#I1
L4
	h	#I0
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
