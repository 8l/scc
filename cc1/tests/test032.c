/* See LICENSE file for copyright and license details. */

/*
name: TEST032
description: test special characters @ and $ in macro definitions
error:
output:
G3	I	F	"main
{
\
V6	K	#N2C
Y7	V6	"	(
	#"This is a string $ or # or ##and it is ok !
	#K00
)
A5	P	"p
	A5	Y7	'P	:P
	h	A5	#P0	!I
}
*/

#define M1(x) "This is a string $ or # or ##" ## #x

int
main(void)
{
	char *p = M1(and it is ok!);

	return p != 0;
}

