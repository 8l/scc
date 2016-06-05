/* See LICENSE file for copyright and license details. */

/*
name: TEST038
description: Basic test for tentative definitions
error:
test038.c:45: error: redeclaration of 'x'
output:
G1	I	"x
G1	I	"x	(
	#I0
)
X3	I	F	"main
G5	P	F	"foo
{
\
	h	X3	'P
}
G3	I	F	"main
{
\
	G1	#I0	:I
	h	G1
}
*/

int x;
int x = 0;
int x;

int main();

void *
foo()
{
	return &main;
}

int
main()
{
	x = 0;
	return x;
}
int x = 1;
