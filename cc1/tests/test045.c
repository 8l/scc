/* See LICENSE file for copyright and license details. */

/*
name: TEST045
description: Basic test of initializers
error:
output:
G1	I	"x	(
	#I5
)
G3	I	F	"main
{
\
	y	L4	G1	#I5	=I
	h	#I1
L4
	h	#I0
}
*/



int x = 5;

int
main()
{
	if(x != 5) 
		return 1;
	return 0;
}
