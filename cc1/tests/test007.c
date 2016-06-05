/* See LICENSE file for copyright and license details. */

/*
name: TEST007
description: basic while test
error:
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#IA	:I
	j	L6
	e
L4
	A3	A3	#I1	-I	:I
L6
	y	L4	A3	#I0	!I
	b
L5
	h	A3
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
