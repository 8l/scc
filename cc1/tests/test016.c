/* See LICENSE file for copyright and license details. */

/*
name: TEST016
description: Basic pointer test
error:
test016.c:42: error: redefinition of 'func2'
test016.c:46: error: incompatible types when assigning
output:
G1	I	"g
G3	I	F	"func1
{
\
A4	I	"x
A6	P	"p
	G1	#I1	:I
	A4	#I1	:I
	A6	A4	'P	:P
	A6	@I	#I0	:I
	y	L7	A4	#I0	=I
	h	#I1
L7
	A6	G1	'P	:P
	A6	@I	#I0	:I
	y	L8	A6	#P0	!I
	h	#I1
L8
	h	#I0
}
G9	I	F	"func2
{
\
A10	I	"x
A12	P	"p
A14	P	"pp
	A10	#I1	:I
	A12	A10	'P	:P
	A14	A12	'P	:P
	y	L15	A12	#P0	=I
	A14	@P	@I	#I0	:I
L15
	A12	#P0	:P
	h	A10
}
*/

#line 1

int g;

int
func1(void)
{
	int  x;
	int *p;

	g = 1;
	x = 1;
	p = &x;
	*p = 0;
	if (x)
		return 1;

	p = &g;
	*p = 0;
	if (p == 0)
		return 1;
	return 0;
}

int
func2(void)
{
	int   x;
	int  *p;
	int **pp;

	x = 1;
	p = &x;
	pp = &p;
	if (p != 0)
		**pp = 0;
	p = 0;
	return x;
}

int
func2(void)
{
	char c;
	int *p;

	p = &c;
	return *p;
}
