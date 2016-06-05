/* See LICENSE file for copyright and license details. */

/*
name: TEST009
description: Basic test for loops
error:
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#I0	:I
	j	L6
	e
L4
	A3	A3	#I1	+I	:I
L6
	y	L4	A3	#IA	<I
	b
L5
	y	L7	A3	#IA	=I
	h	#I1
L7
	h	#I0
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

