/* See LICENSE file for copyright and license details. */

/*
name: TEST064
description: Test function alike macro without parenthesis
error:
output:
G5	I	F	"main
{
\
S1	"	#N2	#N1
M2	I	"f	#N0
A6	S1	"s
	A6	M2	.I	#I0	:I
	h	A6	M2	.I
}
*/

#define x f
#define y() f

typedef struct { int f; } S;

int
main()
{
	S s;
	
	s.x = 0;
	return s.y();
}

