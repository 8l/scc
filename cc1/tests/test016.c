/*
name: TEST016
description: Basic pointer test
output:
test016.c:43: error: redefinition of 'func2'
G1	I	g
F1
G2	F1	func1
{
-
A2	I	x
A4	P	p
	G1	#I1	:I
	A2	#I1	:I
	A4	A2	'P	:P
	A4	@I	#I0	:I
	j	L5	A2	#I0	=I
	r	#I1
L5
	A4	G1	'P	:P
	A4	@I	#I0	:I
	j	L6	A4	#P0	!I
	r	#I1
L6
	r	#I0
}
G3	F1	func2
{
-
A1	I	x
A2	P	p
A4	P	pp
	A1	#I1	:I
	A2	A1	'P	:P
	A4	A2	'P	:P
	j	L5	A2	#P0	=I
	A4	@P	@I	#I0	:I
L5
	A2	#P0	:P
	r	A1
}
test016.c:47: error: incompatible types when assigning
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
