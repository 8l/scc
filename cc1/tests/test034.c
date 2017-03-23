/* See LICENSE file for copyright and license details. */

/*
name: TEST034
description: Basic test for incomplete structures
error:
test034.c:48: error: declared variable 'bar' of incomplete type
test034.c:48: error: redeclaration of 'bar'
output:
X3	S2	"x
X5	I	F	"foo
G6	I	F	"main
{
\
X7	S2	"x
	h	X7	'P	#P0	!I
}
S2	"X	#N2	#N1
M9	I	"v	#N0
G5	I	F	"foo
{
\
	X3	M9	.I	#I0	:I
	h	X3	M9	.I
}
X13	S11	"bar2
*/

extern struct X x;
int foo();

int main()
{
	extern struct X x;
	return &x != 0;
}

struct X {int v;};

int foo()
{
	x.v = 0;
	return x.v;
}

typedef struct bar bar;
extern bar bar2;
bar bar;
